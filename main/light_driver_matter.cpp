
#include "light_driver_matter.h"
#include "hal_defs.h"

#include <esp_log.h>

#include <esp_matter.h>
#include <esp_matter_attribute_utils.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "esp_timer.h"

using namespace chip::app::Clusters;
using namespace esp_matter;


/*-----------------------------------------------------------------------------------------------*/
/* global variables                                                                             */
/*-----------------------------------------------------------------------------------------------*/

#define MAX_LIGHTS 2

static light_handle_t light_handles[MAX_LIGHTS];
static uint8_t light_count = 0;

static QueueHandle_t queue = NULL;


/*-----------------------------------------------------------------------------------------------*/
/* functions implementations                                                                             */
/*-----------------------------------------------------------------------------------------------*/


/// @brief - config gpio for the light and store pins the handle
///        - create endpoint attached to node ; store endpoint_id in the handle ; (endpoint type is onoff light)
///        - create onoff cluster, onoff attribute
/// @param relay_pin
/// @param push_btn_pin
/// @param ext_cmd_pin
/// @return
extern "C" light_handle_t * create_light(node_t * light_node, uint8_t relay_pin, uint8_t push_btn_pin, uint8_t ext_cmd_pin)
{
    if (light_count >= MAX_LIGHTS)
    {
        ESP_LOGE("light_driver_matter", "create_light: max nbr of lights (%d) reached", MAX_LIGHTS);
        return nullptr;
    }

    // btn_event queue : shared by all lights, created once ; must exist before push_btn_init() enables its irq
    // (depth sized for 4 anyedge sources : 2 btns + 2 ext cmd lines, all can bounce simultaneously
    //  while the task is stalled in a matter attribute update)
    if (queue == NULL)
    {
        queue = xQueueCreate(32, sizeof(btn_event_t));
    }

    // gpio ; ext_cmd shares push_btn_isr : both just post {pin, level, timestamp} to the queue
    relay_init(relay_pin);
    push_btn_init(push_btn_pin, push_btn_isr);
    ext_cmd_init(ext_cmd_pin, push_btn_isr);

    // light_handle (slot claimed only if endpoint creation succeeds)
    light_handle_t * light_handle = &light_handles[light_count];
    light_handle->push_btn_pin = push_btn_pin;
    light_handle->relay_pin = relay_pin;
    light_handle->ext_cmd_pin = ext_cmd_pin;

    // an on_off_light endpoint config : default attributes values for an onoff_light endpoint
    esp_matter::endpoint::on_off_light::config_t onoff_light_endpoint_cfg;
    onoff_light_endpoint_cfg.on_off.on_off = false; // added information

    // create light endpoint with default config (adds mandatory light endpoint clusters for certification (OnOff, Descriptor, etc.))
    endpoint_t * light_endpoint = esp_matter::endpoint::on_off_light::create(light_node, &onoff_light_endpoint_cfg , ENDPOINT_FLAG_NONE, light_handle);

    // fails e.g. when CONFIG_ESP_MATTER_MAX_DYNAMIC_ENDPOINT_COUNT is too small (root endpoint 0 counts toward it)
    if (light_endpoint == nullptr)
    {
        ESP_LOGE("light_driver_matter", "create_light: endpoint creation failed (check CONFIG_ESP_MATTER_MAX_DYNAMIC_ENDPOINT_COUNT)");
        return nullptr;
    }

    light_count++;

    // store endpoint id in the handle
    uint16_t id = endpoint::get_id(light_endpoint);
    light_handle->endpoint_id = id;

    ESP_LOGI("light_driver_matter", "light created: endpoint_id=%u relay_pin=%u btn_pin=%u ext_cmd_pin=%u", id, relay_pin, push_btn_pin, ext_cmd_pin);

    // return light_handle_t *
    return light_handle;

}



/**
 * @brief sends elt = event = pin,lvl,timestamp to the queue ; push_btn_task receives elt from the queue
 *
 */
extern "C" void push_btn_isr(void *arg)
{
    uint8_t pin = (uint8_t)(intptr_t)arg;
    btn_event_t event;

    event.pin = pin;
    event.level = gpio_get_level((gpio_num_t)pin);
    event.timestamp = xTaskGetTickCountFromISR();

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(queue, &event, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}



/**
 * @brief flip the light's onoff attribute (attribute::update() then drives the relay gpio)
 */
static void light_toggle_onoff(light_handle_t * light)
{
    esp_matter_attr_val_t val = esp_matter_invalid(NULL);
    attribute_t * attribute = attribute::get(light->endpoint_id, OnOff::Id, OnOff::Attributes::OnOff::Id);
    attribute::get_val(attribute, &val);
    val.val.b = !val.val.b;

    ESP_LOGI("light_driver_matter", "toggle onoff : endpoint %u -> %d", light->endpoint_id, (int)val.val.b);

    attribute::update(light->endpoint_id, OnOff::Id, OnOff::Attributes::OnOff::Id, &val);
}



/**
 * @brief handles the 2 local btns (press < 1s = toggle ; both held 10s = factory reset)
 *        and the 2 ext cmd lines (1 press = 1 onoff toggle)
 *
 *        btn debounce : an edge is accepted only if it changes the stable state of the btn
 *        AND arrives at least DEBOUNCE_MS after the last accepted edge
 *
 * @param pvParameters light_ctx_t *
 */
extern "C" void push_btn_task(void *pvParameters)
{
    light_ctx_t * light_ctx = (light_ctx_t *)pvParameters;
    btn_ctx_t * btns[2] = { &light_ctx->btn_1, &light_ctx->btn_2 };
    ext_cmd_ctx_t * exts[2] = { &light_ctx->ext_1, &light_ctx->ext_2 };

    esp_timer_handle_t fctry_reset_timer_handle;
    const esp_timer_create_args_t fctry_reset_timer_args =
    {
        .callback = fctry_reset_timer_cb,
        .arg = NULL,
        .name = "fctry_reset_timer"
    };
    esp_timer_create(&fctry_reset_timer_args, &fctry_reset_timer_handle);

    bool both_pressed = false;

    btn_event_t event;
    while (1)
    {
        if (!xQueueReceive(queue, &event, portMAX_DELAY))
        {
            continue;
        }

        // external command (ext_cmd_1 or ext_cmd_2) ?
        ext_cmd_ctx_t * ext = nullptr;
        for (int i = 0; i < 2; i++)
        {
            if (exts[i]->pin == event.pin)
            {
                ext = exts[i];
                break;
            }
        }
        if (ext != nullptr)
        {
            // 1st edge after a quiet gap = a new press => toggle ; closer edges are the 50Hz
            // pulse train / bounce of the same press => absorbed
            if ((TickType_t)(event.timestamp - ext->last_event) >= pdMS_TO_TICKS(EXT_CMD_SETTLE_MS))
            {
                ESP_LOGI("light_driver_matter", "ext cmd pin %u : press", event.pin);
                light_toggle_onoff(ext->light);
            }

            ext->last_event = event.timestamp;
            continue;
        }

        // find which button (btn_1 or btn_2) this event belongs to
        btn_ctx_t * btn = nullptr;
        for (int i = 0; i < 2; i++)
        {
            if (btns[i]->pin == event.pin)
            {
                btn = btns[i];
                break;
            }
        }
        if (btn == nullptr)
        {
            continue; // event from an unknown pin
        }

        ///// debounce filter

        bool new_pressed = (event.level == 0); // active low : negedge = press

        if (new_pressed == btn->pressed)
        {
            continue; // no state change (bounce echo of an already-accepted edge)
        }
        if ((uint32_t)(event.timestamp - btn->last_edge) * portTICK_PERIOD_MS < DEBOUNCE_MS)
        {
            continue; // too close to the last accepted edge : bounce
        }

        // edge accepted : update stable state
        btn->pressed = new_pressed;
        btn->last_edge = event.timestamp;

        ///// state machine

        if (new_pressed) // press
        {
            btn->press_time = event.timestamp;

            // both btns now held => arm factory-reset timer (10s)
            if (btns[0]->pressed && btns[1]->pressed)
            {
                both_pressed = true;
                btns[0]->in_combo = true; // their upcoming releases must not toggle the lights
                btns[1]->in_combo = true;
                esp_timer_start_once(fctry_reset_timer_handle, FCTRY_RESET_PRESS_DURATION_US);
            }
        }
        else // release
        {
            if (both_pressed)
            {
                // 1 of the 2 simultaneously-held btns is released before 10s : abort the factory reset
                both_pressed = false;
                esp_timer_stop(fctry_reset_timer_handle);
            }

            bool was_combo = btn->in_combo;
            btn->in_combo = false;

            if (!was_combo)
            {
                uint32_t d_ms = (uint32_t)(event.timestamp - btn->press_time) * portTICK_PERIOD_MS;

                if (d_ms < LONG_PRESS_INF_MS) // short press => toggle this btn's light
                {
                    light_toggle_onoff(btn->light);
                }
                // long press (>= LONG_PRESS_INF_MS) on a single btn : no action defined
            }
        }
    }
}



/**
 * @brief executes when fctry_reset_timer expires : do a factory_reset : delete matter data in nvs + restart device
 *
 * @param args
 */
void fctry_reset_timer_cb(void * args)
{
    esp_matter::factory_reset(); // delete matter data in nvs + restart device
}
