
#pragma once

#include <stdint.h>

#include <esp_matter.h>
#include <esp_matter_cluster.h>
#include <esp_matter_endpoint.h>

using namespace esp_matter;
using namespace esp_matter::attribute;
using namespace esp_matter::endpoint;
using namespace chip::app::Clusters;


#define MAX_LIGHTS 2


/*-----------------------------------------------------------------------------------------------*/
/* types                                                                                         */
/*-----------------------------------------------------------------------------------------------*/

typedef struct
{
    uint16_t endpoint_id;
    uint8_t relay_pin;
    uint8_t push_btn_pin;
    uint8_t ext_cmd_pin;

} light_handle_t;


/*-----------------------------------------------------------------------------------------------*/
/* headers                                                                                       */
/*-----------------------------------------------------------------------------------------------*/

/// @brief - config the relay gpio for the light, store pins in the handle
///        - create a Matter onoff_light endpoint attached to light_node ; store endpoint_id in the handle
/// @param light_node
/// @param relay_pin
/// @param push_btn_pin
/// @param ext_cmd_pin
/// @return light_handle_t * on success, nullptr if MAX_LIGHTS is reached or endpoint creation fails
extern "C" light_handle_t * create_light(node_t * light_node, uint8_t relay_pin, uint8_t push_btn_pin, uint8_t ext_cmd_pin);


/// @brief flip the light's onoff attribute (attribute::update() then drives the relay gpio via app_attribute_update_cb)
void matter_light_toggle_onoff(light_handle_t * light);
