
#include "light_driver_matter.h"

#include <esp_matter.h>
#include <esp_matter_attribute_utils.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

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
    if (queue == NULL)
    {
        queue = xQueueCreate(10, sizeof(btn_event_t));
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
 * @brief sends pin nbr to the queue ; push_btn_task receives elt from the queue
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
 * @brief flips the light's onoff attribute :
 *        attribute::update() triggers app_driver_attribute_update() (PRE_UPDATE) which drives the relay gpio
 *
 * @param light
 */
static void light_toggle_onoff(light_handle_t * light)
{
    esp_matter_attr_val_t val = esp_matter_invalid(NULL);
    attribute_t * attribute = attribute::get(light->endpoint_id, OnOff::Id, OnOff::Attributes::OnOff::Id);
    attribute::get_val(attribute, &val);
    val.val.b = !val.val.b;
    attribute::update(light->endpoint_id, OnOff::Id, OnOff::Attributes::OnOff::Id, &val);
}



/**
 * @brief handles btn_1 / btn_2 press (any press < 1s toggles the corresponding light's onoff attribute)
 *        and simultaneous press of both btns held 10s (factory reset)
 *
 *        also handles ext_cmd_1 / ext_cmd_2 (external push btns) : the external transistor toggles
 *        the relay directly in hw and drives the ext_cmd pin to 1 ; on the accepted posedge we flip
 *        the onoff attribute so matter stays in sync (the relay gpio is re-driven to the same state)
 *
 *        debounce : an edge is ACCEPTED only if it changes the stable state of the btn AND arrives
 *        at least DEBOUNCE_MS after the last accepted edge ; bounce edges are thus dropped without
 *        corrupting the state machine (a bounce posedge used to abort the factory-reset timer)
 *
 * @param pvParameters light_ctx_t * : {btn_1, btn_2}
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
            ///// debounce filter (same rule as the btns : state change + min delay since last accepted edge)

            bool new_active = (event.level == 1); // active high : posedge = external press

            if (new_active == ext->active)
            {
                continue; // no state change (bounce echo of an already-accepted edge)
            }
            if ((uint32_t)(event.timestamp - ext->last_edge) * portTICK_PERIOD_MS < DEBOUNCE_MS)
            {
                continue; // too close to the last accepted edge : bounce
            }

            // edge accepted : update stable state
            ext->active = new_active;
            ext->last_edge = event.timestamp;

            if (new_active) // posedge = the moment the hw toggles the relay : re-sync the matter attribute
            {
                light_toggle_onoff(ext->light);
            }
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
