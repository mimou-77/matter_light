
#include <esp_err.h>
#include <esp_log.h>
#include <nvs_flash.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <esp_matter.h>
#include <esp_matter_cluster.h>
#include <esp_matter_endpoint.h>

#include <esp_matter_ota.h>

#include <common_macros.h>

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
#include <esp_openthread.h>
#include <esp_openthread_types.h>
#include <platform/ESP32/OpenthreadLauncher.h>
#endif

#include <app/server/CommissioningWindowManager.h>
#include <app/server/Server.h>

#include "app_logic_matter.h"
#include "light_driver_matter.h"

#include "hal_led.h"

#include "hal_defs.h"

using namespace esp_matter;
using namespace esp_matter::attribute;
using namespace esp_matter::endpoint;
using namespace chip::app::Clusters;


static const char *TAG = "app_main";





extern "C" void app_main()
{

    esp_err_t err = ESP_OK;

    /* Initialize the ESP NVS layer */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* Create a Matter node and add the mandatory Root Node device type on endpoint 0 */
    node::config_t node_config;

    // attach app_attribute_update_cb and app_identification_cb to the node
    node_t *node = node::create(&node_config, app_attribute_update_cb, app_identification_cb);
    ABORT_APP_ON_FAILURE(node != nullptr, ESP_LOGE(TAG, "Failed to create Matter node"));


    // create 2 onoff_light endpoints, each with its own relay + push button + external command input ;
    // light_x_handle contains relay_pin + push_btn_pin + ext_cmd_pin + endpoint_id
    light_handle_t * light_1_handle = create_light(node, (uint8_t)RELAY_1_PIN, (uint8_t)PUSH_BTN_1_PIN, (uint8_t)EXT_CMD_1_PIN);
    ABORT_APP_ON_FAILURE(light_1_handle != nullptr, ESP_LOGE(TAG, "Failed to create light 1"));

    light_handle_t * light_2_handle = create_light(node, (uint8_t)RELAY_2_PIN, (uint8_t)PUSH_BTN_2_PIN, (uint8_t)EXT_CMD_2_PIN);
    ABORT_APP_ON_FAILURE(light_2_handle != nullptr, ESP_LOGE(TAG, "Failed to create light 2"));


    // status led : blinks while advertising for commissioning (see app_event_cb)
    led_init((uint8_t)LED_PIN);


    // config thread
    #if CHIP_DEVICE_CONFIG_ENABLE_THREAD
        /* Set OpenThread platform config */
        esp_openthread_platform_config_t config = {
            .radio_config = ESP_OPENTHREAD_DEFAULT_RADIO_CONFIG(),
            .host_config = ESP_OPENTHREAD_DEFAULT_HOST_CONFIG(),
            .port_config = ESP_OPENTHREAD_DEFAULT_PORT_CONFIG(),
        };
        set_openthread_platform_config(&config);
    #endif


   
    /* Matter start */
    err = esp_matter::start(app_event_cb);
    ABORT_APP_ON_FAILURE(err == ESP_OK, ESP_LOGE(TAG, "Failed to start Matter, err:%d", err));



    /* push btn task : handle long press + short press + external commands => updates matter attributes of light endpoint*/
    static light_ctx_t light_ctx = {
        .btn_1 = { .pin = PUSH_BTN_1_PIN, .light = light_1_handle, .press_time = 0 },
        .btn_2 = { .pin = PUSH_BTN_2_PIN, .light = light_2_handle, .press_time = 0 },
        .ext_1 = { .pin = EXT_CMD_1_PIN, .light = light_1_handle },
        .ext_2 = { .pin = EXT_CMD_2_PIN, .light = light_2_handle },
    };
    xTaskCreate(push_btn_task, "push_btn_task", 4096, &light_ctx, 5, NULL);
}

