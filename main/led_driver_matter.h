
#pragma once
#ifndef __LED_DRIVER_MATTER_H__
#define __LED_DRIVER_MATTER_H__



#include "hal_led.h"

#include <esp_matter.h>
#include <esp_matter_cluster.h>
#include <esp_matter_endpoint.h>

using namespace esp_matter;
using namespace esp_matter::attribute;
using namespace esp_matter::endpoint;
using namespace chip::app::Clusters;


/*-----------------------------------------------------------------------------------------------*/
/* types                                                                                         */
/*-----------------------------------------------------------------------------------------------*/


typedef struct 
{
    uint16_t endpoint_id;
    uint8_t pin;

} led_handle_t;



/*-----------------------------------------------------------------------------------------------*/
/* headers                                                                                       */
/*-----------------------------------------------------------------------------------------------*/



/// @brief - config gpio for the led and store pin the handle
///        - create endpoint attached to node ; store endpoint_id in the handle
///        - create onoff cluster, onoff attribute
/// @param led_node 
/// @param led_pin 
/// @return 
extern "C" led_handle_t * create_led(node_t * led_node, uint8_t led_pin);




#endif // __LED_DRIVER_MATTER_H__