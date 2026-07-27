
#include "matter_callbacks.h"
#include "matter_light_endpoint.h"

#include <esp_err.h>
#include <esp_log.h>

#include <esp_matter.h>
#include <esp_matter_ota.h>

#include <common_macros.h>

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
#include <platform/ESP32/OpenthreadLauncher.h>
#endif

#include <app/server/CommissioningWindowManager.h>
#include <app/server/Server.h>

#include "hal_relay.h"
#include "hal_led.h"
#include "hal_defs.h"

using namespace esp_matter;
using namespace esp_matter::attribute;
using namespace esp_matter::endpoint;
using namespace chip::app::Clusters;


/*-----------------------------------------------------------------------------------------------*/
/* global variables                                                                             */
/*-----------------------------------------------------------------------------------------------*/

static const char *TAG = "matter_callbacks";

constexpr auto k_timeout_seconds = 300; // advertisement ble lasts 5s then closes


/*-----------------------------------------------------------------------------------------------*/
/* functions implementations                                                                             */
/*-----------------------------------------------------------------------------------------------*/

/// @brief logs network events
/// @param event
/// @param arg
void app_event_cb(const chip::DeviceLayer::ChipDeviceEvent *event, intptr_t arg)
{
    switch (event->Type) {
    case chip::DeviceLayer::DeviceEventType::kInterfaceIpAddressChanged:
        ESP_LOGI(TAG, "Interface IP Address changed");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningComplete:
        ESP_LOGI(TAG, "Commissioning complete");
        break;

    case chip::DeviceLayer::DeviceEventType::kFailSafeTimerExpired:
        ESP_LOGI(TAG, "Commissioning failed, fail safe timer expired");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningSessionStarted:
        ESP_LOGI(TAG, "Commissioning session started");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningSessionStopped:
        ESP_LOGI(TAG, "Commissioning session stopped");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningWindowOpened:
        ESP_LOGI(TAG, "Commissioning window opened");
        // advertising for commissioning starts : blink the status led rapidly
        led_start_blink((uint8_t)LED_PIN, LED_ADV_BLINK_INTERVAL_MS);
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningWindowClosed:
        ESP_LOGI(TAG, "Commissioning window closed");
        // advertising stops (paired or not) : stop blinking the status led
        led_stop_blink((uint8_t)LED_PIN);
        break;

    case chip::DeviceLayer::DeviceEventType::kFabricRemoved: {
        ESP_LOGI(TAG, "Fabric removed successfully");
        if (chip::Server::GetInstance().GetFabricTable().FabricCount() == 0) {
            chip::CommissioningWindowManager &commissionMgr = chip::Server::GetInstance().GetCommissioningWindowManager();
            constexpr auto kTimeoutSeconds = chip::System::Clock::Seconds16(k_timeout_seconds);
            if (!commissionMgr.IsCommissioningWindowOpen()) {
                /* After removing last fabric, this example does not remove the Wi-Fi credentials
                 * and still has IP connectivity so, only advertising on DNS-SD.
                 */
                CHIP_ERROR err = commissionMgr.OpenBasicCommissioningWindow(kTimeoutSeconds,
                                                                            chip::CommissioningWindowAdvertisement::kDnssdOnly);
                if (err != CHIP_NO_ERROR) {
                    ESP_LOGE(TAG, "Failed to open commissioning window, err:%" CHIP_ERROR_FORMAT, err.Format());
                }
            }
        }
        break;
    }

    case chip::DeviceLayer::DeviceEventType::kFabricWillBeRemoved:
        ESP_LOGI(TAG, "Fabric will be removed");
        break;

    case chip::DeviceLayer::DeviceEventType::kFabricUpdated:
        ESP_LOGI(TAG, "Fabric is updated");
        break;

    case chip::DeviceLayer::DeviceEventType::kFabricCommitted:
        ESP_LOGI(TAG, "Fabric is committed");
        break;

    case chip::DeviceLayer::DeviceEventType::kBLEDeinitialized:
        ESP_LOGI(TAG, "BLE deinitialized and memory reclaimed");
        break;

    default:
        break;
    }
}



/// @brief called when clients interact with the Identify Cluster
/// @param type
/// @param endpoint_id
/// @param effect_id
/// @param effect_variant
/// @param priv_data
/// @return
esp_err_t app_identification_cb(identification::callback_type_t type, uint16_t endpoint_id, uint8_t effect_id,
                                uint8_t effect_variant, void *priv_data)
{
    ESP_LOGI(TAG, "Identification callback: type: %u, effect: %u, variant: %u", type, effect_id, effect_variant);
    return ESP_OK;
}



/// @brief updates hw for driver_handle (currently always a light_handle_t *) :
///        if cluster is OnOff and attribute is OnOff, drive that light's relay pin to the new value
/// @param driver_handle light_handle_t * (endpoint_id + relay_pin + push_btn_pin + ext_cmd_pin)
/// @param endpoint_id
/// @param cluster_id
/// @param attribute_id
/// @param val
/// @return
static esp_err_t app_driver_attribute_update(void *driver_handle, uint16_t endpoint_id, uint32_t cluster_id,
                                              uint32_t attribute_id, esp_matter_attr_val_t *val)
{
    esp_err_t err = ESP_OK;

    if (cluster_id == OnOff::Id) // cluster onoff will be used to represent the light fns
    {
        if (attribute_id == OnOff::Attributes::OnOff::Id) // attr onoff
        {
            bool onoff_val = val->val.b;

            light_handle_t * light_handle_ptr = (light_handle_t *)(driver_handle);
            relay_set(light_handle_ptr->relay_pin, onoff_val);
        }
    }

    return err;
}



/// @brief called when an attribute value changes ; updates hw accordingly
/// @param type
/// @param endpoint_id
/// @param cluster_id
/// @param attribute_id
/// @param val
/// @param priv_data
/// @return
esp_err_t app_attribute_update_cb(attribute::callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id,
                                  uint32_t attribute_id, esp_matter_attr_val_t *val, void *priv_data)
{
    esp_err_t err = ESP_OK;

    if (type == PRE_UPDATE) {
        err = app_driver_attribute_update(priv_data, endpoint_id, cluster_id, attribute_id, val);
    }

    return err;
}
