
#pragma once
#ifndef __APP_DRIVER_MATTER_H__
#define __APP_DRIVER_MATTER_H__

#include <stdlib.h>
#include <string.h>

#include "driver/gpio.h"
#include <esp_log.h>

#include <esp_matter.h>
#include "esp_matter_attribute_utils.h"


#include "light_driver_matter.h"

using namespace chip::app::Clusters;
using namespace esp_matter;

/*-----------------------------------------------------------------------------------------------*/
/* types                                                                                         */
/*-----------------------------------------------------------------------------------------------*/

typedef void * app_driver_handle_t;


/*-----------------------------------------------------------------------------------------------*/
/* headers                                                                                       */
/*-----------------------------------------------------------------------------------------------*/

/// @brief called by "app_attribute_update_cb" ;
///         updates hw of driver_handle > endpoint > cluster > attr with val
///         HERE :
///             - (light_x) : if cluster id is onoff and attribute is onoff => relay_set(relay_pin, val)
///                         ; relay_pin from driver_handle
/// @param driver_handle can be a light_handle_t, ...
///                         light_handle_t = {endpoint_id, relay_pin, push_btn_pin}
/// @param endpoint_id 
/// @param cluster_id 
/// @param attribute_id 
/// @param val 
/// @return 
esp_err_t app_driver_attribute_update(app_driver_handle_t driver_handle, uint16_t endpoint_id, uint32_t cluster_id,
                                      uint32_t attribute_id, esp_matter_attr_val_t *val);











#endif // __APP_DRIVER_MATTER_H__