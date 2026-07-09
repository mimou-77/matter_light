

#include "hal_relay.h" 


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*-----------------------------------------------------------------------------------------------*/
/* global variables                                                                             */
/*-----------------------------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------------------------------*/
/* functions implementations                                                                             */
/*-----------------------------------------------------------------------------------------------*/

void relay_set(uint8_t pin, bool lvl)
{
    gpio_set_level((gpio_num_t)pin, (uint32_t)lvl);
}

void relay_toggle(uint8_t pin)
{
    uint32_t lvl = gpio_get_level((gpio_num_t)pin);
    gpio_set_level((gpio_num_t)pin, !lvl);
}




void relay_init(uint8_t pin)
{
    gpio_reset_pin((gpio_num_t)pin);

    gpio_config_t led_pin_cfg = 
    {
        .pin_bit_mask = (1ULL << pin),
        .mode = GPIO_MODE_INPUT_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&led_pin_cfg);

   // set to 0
   relay_set(pin, 0);

}
