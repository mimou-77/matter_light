

#pragma once

#include <esp_err.h>
#include <esp_matter.h>


#include <platform/CHIPDeviceLayer.h>
#include <app/server/Server.h>

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
#include "esp_openthread_types.h"
#endif

#include "app_driver_matter.h"

using namespace esp_matter;
using namespace esp_matter::attribute;

#include <esp_matter_identify.h>



/*-----------------------------------------------------------------------------------------------*/
// Macros
/*-----------------------------------------------------------------------------------------------*/
#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
#define ESP_OPENTHREAD_DEFAULT_RADIO_CONFIG()                                           \
    {                                                                                   \
        .radio_mode = RADIO_MODE_NATIVE,                                                \
    }

#define ESP_OPENTHREAD_DEFAULT_HOST_CONFIG()                                            \
    {                                                                                   \
        .host_connection_mode = HOST_CONNECTION_MODE_NONE,                              \
    }

#define ESP_OPENTHREAD_DEFAULT_PORT_CONFIG()                                            \
    {                                                                                   \
        .storage_partition_name = "nvs", .netif_queue_size = 10, .task_queue_size = 10, \
    }
#endif


/*-----------------------------------------------------------------------------------------------*/
/* headers                                                                             */
/*-----------------------------------------------------------------------------------------------*/

/// @brief logs network events 
/// @param event 
/// @param arg 
void app_event_cb(const chip::DeviceLayer::ChipDeviceEvent *event, intptr_t arg);



/// @brief called when an attr val changes ; mostly updates hw
///             calls app_driver_attribute_update_cb() (from app_driver_matter.cpp)
/// @param type 
/// @param endpoint_id 
/// @param cluster_id 
/// @param attribute_id 
/// @param val 
/// @param priv_data 
/// @return 
esp_err_t app_attribute_update_cb(esp_matter::attribute::callback_type_t type, uint16_t endpoint_id,
                                  uint32_t cluster_id, uint32_t attribute_id, esp_matter_attr_val_t *val,
                                  void *priv_data);



/// @brief executes when a client interacts with "identify" cluster
/// @param type 
/// @param endpoint_id 
/// @param effect_id 
/// @param effect_variant 
/// @param priv_data 
/// @return 
esp_err_t app_identification_cb(esp_matter::identification::callback_type_t type, uint16_t endpoint_id,
                                uint8_t effect_id, uint8_t effect_variant, void *priv_data);



