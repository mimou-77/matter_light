

#include "hal_ext_cmd.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*-----------------------------------------------------------------------------------------------*/
/* global variables                                                                             */
/*-----------------------------------------------------------------------------------------------*/

// gpio isr service is chip-wide : shared with hal_push_btn.c, must be installed only once
extern bool isr_service_is_installed;

/*-----------------------------------------------------------------------------------------------*/
/* functions implementations                                                                             */
/*-----------------------------------------------------------------------------------------------*/

bool ext_cmd_get(uint8_t pin)
{
    int lvl = gpio_get_level((gpio_num_t)pin);

    return ((bool)lvl);
}


void ext_cmd_init(uint8_t pin, ext_cmd_isr_t isr)
{
    gpio_reset_pin((gpio_num_t)pin);

    gpio_config_t ext_cmd_pin_cfg =
    {
        .pin_bit_mask = (1ULL << pin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE, // open-collector stage : idle at 1, press pulls to 0
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE
    };

    gpio_config(&ext_cmd_pin_cfg);

    if (!isr_service_is_installed)
    {
        gpio_install_isr_service(0);
        isr_service_is_installed = 1;
    }

    gpio_isr_handler_add((gpio_num_t)pin, isr, (void *)(intptr_t)pin); // each time the intr happens, the corresponding pin nbr will be sent as arg to the isr

}
