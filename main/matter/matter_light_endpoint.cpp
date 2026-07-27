
#include "matter_light_endpoint.h"

#include "hal_relay.h"

#include <esp_log.h>
#include <esp_matter_attribute_utils.h>

using namespace chip::app::Clusters;
using namespace esp_matter;


/*-----------------------------------------------------------------------------------------------*/
/* global variables                                                                             */
/*-----------------------------------------------------------------------------------------------*/

static const char *TAG = "matter_light_endpoint";

static light_handle_t light_handles[MAX_LIGHTS];
static uint8_t light_count = 0;


/*-----------------------------------------------------------------------------------------------*/
/* functions implementations                                                                             */
/*-----------------------------------------------------------------------------------------------*/

extern "C" light_handle_t * create_light(node_t * light_node, uint8_t relay_pin, uint8_t push_btn_pin, uint8_t ext_cmd_pin)
{
    if (light_count >= MAX_LIGHTS)
    {
        ESP_LOGE(TAG, "create_light: max nbr of lights (%d) reached", MAX_LIGHTS);
        return nullptr;
    }

    relay_init(relay_pin);

    // light_handle (slot claimed only if endpoint creation succeeds)
    light_handle_t * light_handle = &light_handles[light_count];
    light_handle->push_btn_pin = push_btn_pin;
    light_handle->relay_pin = relay_pin;
    light_handle->ext_cmd_pin = ext_cmd_pin;

    // an on_off_light endpoint config : default attributes values for an onoff_light endpoint
    esp_matter::endpoint::on_off_light::config_t onoff_light_endpoint_cfg;
    onoff_light_endpoint_cfg.on_off.on_off = false; // added information

    // create light endpoint with default config (adds mandatory light endpoint clusters for certification (OnOff, Descriptor, etc.))
    endpoint_t * light_endpoint = esp_matter::endpoint::on_off_light::create(light_node, &onoff_light_endpoint_cfg , ENDPOINT_FLAG_NONE, light_handle);

    // fails e.g. when CONFIG_ESP_MATTER_MAX_DYNAMIC_ENDPOINT_COUNT is too small (root endpoint 0 counts toward it)
    if (light_endpoint == nullptr)
    {
        ESP_LOGE(TAG, "create_light: endpoint creation failed (check CONFIG_ESP_MATTER_MAX_DYNAMIC_ENDPOINT_COUNT)");
        return nullptr;
    }

    light_count++;

    // store endpoint id in the handle
    uint16_t id = endpoint::get_id(light_endpoint);
    light_handle->endpoint_id = id;

    ESP_LOGI(TAG, "light created: endpoint_id=%u relay_pin=%u btn_pin=%u ext_cmd_pin=%u", id, relay_pin, push_btn_pin, ext_cmd_pin);

    return light_handle;
}



void matter_light_toggle_onoff(light_handle_t * light)
{
    esp_matter_attr_val_t val = esp_matter_invalid(NULL);
    attribute_t * attribute = attribute::get(light->endpoint_id, OnOff::Id, OnOff::Attributes::OnOff::Id);
    attribute::get_val(attribute, &val);
    val.val.b = !val.val.b;

    ESP_LOGI(TAG, "toggle onoff : endpoint %u -> %d", light->endpoint_id, (int)val.val.b);

    attribute::update(light->endpoint_id, OnOff::Id, OnOff::Attributes::OnOff::Id, &val);
}
