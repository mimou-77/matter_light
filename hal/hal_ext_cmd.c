

#include "hal_ext_cmd.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <esp_log.h>

/*-----------------------------------------------------------------------------------------------*/
/* global variables                                                                             */
/*-----------------------------------------------------------------------------------------------*/

// gpio isr service is chip-wide : shared with hal_push_btn.c, must be installed only once
extern bool isr_service_is_installed;

static const char *TAG = "hal_ext_cmd";

/*-----------------------------------------------------------------------------------------------*/
/* functions implementations                                                                             */
/*-----------------------------------------------------------------------------------------------*/

bool ext_cmd_get(uint8_t pin)
{
    int lvl = gpio_get_level((gpio_num_t)pin);

    return ((bool)lvl);
}


/// @brief the ext cmd stage is a transistor whose input branch is switched by 220Vac : its output
///        topology (open-collector pulling LOW when pressed, or sourcing HIGH when pressed) is
///        detected here by probing the line with both pulls :
///        - if the level follows the pull, the line is high-impedance at rest (open-collector
///          style stage) => keep a pull-up, the stage will pull the line LOW when pressed
///        - if the level is the same under both pulls, the line is driven (or externally pulled)
///          at rest => keep a weak pull toward that rest level for noise immunity
///        the toggle logic in light_driver_matter samples the idle level at task start, so it
///        adapts to either polarity automatically
void ext_cmd_init(uint8_t pin, ext_cmd_isr_t isr)
{
    gpio_reset_pin((gpio_num_t)pin);

    gpio_config_t ext_cmd_pin_cfg =
    {
        .pin_bit_mask = (1ULL << pin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE // enabled at the end, once the pull is final
    };

    gpio_config(&ext_cmd_pin_cfg);

    // probe the rest level under both pulls (the device boots with the ext btns released)
    vTaskDelay(pdMS_TO_TICKS(10)); // let the line settle on the new pull
    int lvl_pullup = gpio_get_level((gpio_num_t)pin);

    gpio_set_pull_mode((gpio_num_t)pin, GPIO_PULLDOWN_ONLY);
    vTaskDelay(pdMS_TO_TICKS(10));
    int lvl_pulldown = gpio_get_level((gpio_num_t)pin);

    if (lvl_pullup != lvl_pulldown)
    {
        // high-impedance at rest : open-collector style stage => pull-up, active low
        gpio_set_pull_mode((gpio_num_t)pin, GPIO_PULLUP_ONLY);
        ESP_LOGI(TAG, "pin %u : floating at rest -> pull-up (stage assumed open-collector, presses pull the line LOW)", pin);
    }
    else
    {
        // driven (or externally pulled) at rest : weak pull toward the observed rest level
        gpio_set_pull_mode((gpio_num_t)pin, lvl_pullup ? GPIO_PULLUP_ONLY : GPIO_PULLDOWN_ONLY);
        ESP_LOGI(TAG, "pin %u : driven at rest (lvl=%d) -> pull toward rest level", pin, lvl_pullup);
    }

    gpio_set_intr_type((gpio_num_t)pin, GPIO_INTR_ANYEDGE);

    if (!isr_service_is_installed)
    {
        gpio_install_isr_service(0);
        isr_service_is_installed = 1;
    }

    gpio_isr_handler_add((gpio_num_t)pin, isr, (void *)(intptr_t)pin); // each time the intr happens, the corresponding pin nbr will be sent as arg to the isr

}
