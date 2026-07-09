

#include "hal_led.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_timer.h"

/*-----------------------------------------------------------------------------------------------*/
/* global variables                                                                             */
/*-----------------------------------------------------------------------------------------------*/

static esp_timer_handle_t blink_timer_handle = NULL;
static bool blink_lvl = 0; // sw toggle state : the led pin is OUTPUT-only, gpio_get_level() on it always reads 0


/*-----------------------------------------------------------------------------------------------*/
/* functions implementations                                                                             */
/*-----------------------------------------------------------------------------------------------*/

void led_set(uint8_t pin, bool lvl)
{
    gpio_set_level((gpio_num_t)pin, (uint32_t)lvl);
}




void led_init(uint8_t pin)
{
    gpio_reset_pin((gpio_num_t)pin);

    gpio_config_t led_pin_cfg = 
    {
        .pin_bit_mask = (1ULL << pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&led_pin_cfg);

    // blink once for 1s

    blink_led_for_duration_ms(pin, 1000);

}



void blink_led_for_duration_ms(uint8_t pin, uint16_t duration_ms)
{
    led_set(pin, 1);

    vTaskDelay(duration_ms/portTICK_PERIOD_MS);

    led_set(pin, 0);
}



static void led_blink_timer_cb(void * arg)
{
    uint8_t pin = (uint8_t)(intptr_t)arg;
    blink_lvl = !blink_lvl;
    led_set(pin, blink_lvl);
}



void led_start_blink(uint8_t pin, uint32_t interval_ms)
{
    if (blink_timer_handle != NULL)
    {
        return; // already blinking
    }

    const esp_timer_create_args_t blink_timer_args =
    {
        .callback = led_blink_timer_cb,
        .arg = (void *)(intptr_t)pin,
        .name = "led_blink_timer"
    };
    esp_timer_create(&blink_timer_args, &blink_timer_handle);

    blink_lvl = 1;
    led_set(pin, blink_lvl); // turn on immediately, then toggle every interval_ms
    esp_timer_start_periodic(blink_timer_handle, (uint64_t)interval_ms * 1000);
}



void led_stop_blink(uint8_t pin)
{
    if (blink_timer_handle == NULL)
    {
        return; // not blinking
    }

    esp_timer_stop(blink_timer_handle);
    esp_timer_delete(blink_timer_handle);
    blink_timer_handle = NULL;

    led_set(pin, 0);
}
