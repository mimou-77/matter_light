


#include "led_driver_matter.h"

#include <esp_matter.h>

using namespace chip::app::Clusters;
using namespace esp_matter;


/*-----------------------------------------------------------------------------------------------*/
/* global variables                                                                             */
/*-----------------------------------------------------------------------------------------------*/

static led_handle_t led_handle; //led_handle_t * is NULL by default => we rather use led_handle_t

/*-----------------------------------------------------------------------------------------------*/
/* functions implementations                                                                             */
/*-----------------------------------------------------------------------------------------------*/

/// @brief - config gpio for the led and store pin the handle
///        - create endpoint attached to node ; store endpoint_id in the handle
///        - create onoff cluster, onoff attribute
/// @param led_node 
/// @param led_pin 
/// @return 
led_handle_t *create_led(node_t *led_node, uint8_t led_pin)
{

    led_init(led_pin); // config gpio + blink 1s

    led_handle.pin = led_pin;

    // create endpoint 
    
    // an on_off_light endpoint config : default attributes values for an onoff_light endpoint
    esp_matter::endpoint::on_off_light::config_t onoff_led_endpoint_cfg;
    onoff_led_endpoint_cfg.on_off.on_off = false; // added information

    // create led endpoint with default config (adds mandatory led endpoint clusters for certification (OnOff, Descriptor, etc.))
    endpoint_t * led_endpoint = esp_matter::endpoint::on_off_light::create(led_node, &onoff_led_endpoint_cfg , ENDPOINT_FLAG_NONE, &led_handle);

    // store endpoint id in the handle
    uint16_t id = endpoint::get_id(led_endpoint); 
    led_handle.endpoint_id = id;
 
    // return led_handle_t *
    return &led_handle;

}
