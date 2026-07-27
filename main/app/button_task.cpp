
#include "button_task.h"
#include "matter_light_endpoint.h"

#include "hal_push_btn.h"
#include "hal_ext_cmd.h"

#include <esp_log.h>
#include <esp_matter.h>

#include "freertos/queue.h"
#include "freertos/task.h"

#include "esp_timer.h"


/*-----------------------------------------------------------------------------------------------*/
/* global variables                                                                             */
/*-----------------------------------------------------------------------------------------------*/

static const char *TAG = "button_task";

#define FCTRY_RESET_PRESS_DURATION_US (10 * 1000000) // 10s
#define DEBOUNCE_MS 30 // min delay between 2 accepted edges on a btn (bounce filter) ; NOT a min press duration : any press >= 30ms toggles
#define LONG_PRESS_INF_MS 1000 // 1s
#define EXT_CMD_SETTLE_MS 100 // ext cmd edges closer than this belong to the same press (50Hz pulses / bounce) ; a later edge = a new press

// btn_event queue : shared by all lights, created once ; must exist before push_btn_init() enables its irq
// (depth sized for 4 anyedge sources : 2 btns + 2 ext cmd lines, all can bounce simultaneously
//  while the task is stalled in a matter attribute update)
static QueueHandle_t queue = NULL;


/*-----------------------------------------------------------------------------------------------*/
/* functions implementations                                                                             */
/*-----------------------------------------------------------------------------------------------*/

/**
 * @brief sends elt = event = pin,lvl,timestamp to the queue ; push_btn_task receives elt from the queue
 */
static void push_btn_isr(void *arg)
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



void button_task_register_light_io(uint8_t push_btn_pin, uint8_t ext_cmd_pin)
{
    if (queue == NULL)
    {
        queue = xQueueCreate(32, sizeof(btn_event_t));
    }

    // gpio ; ext_cmd shares push_btn_isr : both just post {pin, level, timestamp} to the queue
    push_btn_init(push_btn_pin, push_btn_isr);
    ext_cmd_init(ext_cmd_pin, push_btn_isr);
}



/**
 * @brief executes when fctry_reset_timer expires : do a factory_reset : delete matter data in nvs + restart device
 *
 * @param args
 */
static void fctry_reset_timer_cb(void * args)
{
    esp_matter::factory_reset(); // delete matter data in nvs + restart device
}



extern "C" void push_btn_task(void *pvParameters)
{
    light_ctx_t * light_ctx = (light_ctx_t *)pvParameters;

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
        for (int i = 0; i < MAX_LIGHTS; i++)
        {
            if (light_ctx->exts[i].pin == event.pin)
            {
                ext = &light_ctx->exts[i];
                break;
            }
        }
        if (ext != nullptr)
        {
            // 1st edge after a quiet gap = a new press => toggle ; closer edges are the 50Hz
            // pulse train / bounce of the same press => absorbed
            if ((TickType_t)(event.timestamp - ext->last_event) >= pdMS_TO_TICKS(EXT_CMD_SETTLE_MS))
            {
                ESP_LOGI(TAG, "ext cmd pin %u : press", event.pin);
                matter_light_toggle_onoff(ext->light);
            }

            ext->last_event = event.timestamp;
            continue;
        }

        // find which button (btn_1 or btn_2) this event belongs to
        btn_ctx_t * btn = nullptr;
        for (int i = 0; i < MAX_LIGHTS; i++)
        {
            if (light_ctx->btns[i].pin == event.pin)
            {
                btn = &light_ctx->btns[i];
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
            if (light_ctx->btns[0].pressed && light_ctx->btns[1].pressed)
            {
                both_pressed = true;
                light_ctx->btns[0].in_combo = true; // their upcoming releases must not toggle the lights
                light_ctx->btns[1].in_combo = true;
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
                    matter_light_toggle_onoff(btn->light);
                }
                // long press (>= LONG_PRESS_INF_MS) on a single btn : no action defined
            }
        }
    }
}
