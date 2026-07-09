

#include "app_driver_matter.h"


/*-----------------------------------------------------------------------------------------------*/
/* global variables                                                                             */
/*-----------------------------------------------------------------------------------------------*/

static const char *TAG = "app_driver_matter";

/*-----------------------------------------------------------------------------------------------*/
/* functions implementations                                                                             */
/*-----------------------------------------------------------------------------------------------*/


esp_err_t app_driver_attribute_update(app_driver_handle_t driver_handle, uint16_t endpoint_id, uint32_t cluster_id,
                                      uint32_t attribute_id, esp_matter_attr_val_t *val)
{
    esp_err_t err = ESP_OK;


    /////// light : set relay on/off

    if (cluster_id == OnOff::Id) // cluster onoff will be used to represent the light fns
    {
        if (attribute_id == OnOff::Attributes::OnOff::Id) // attr onoff
        {

            // attr onoff changed : get its val
            bool onoff_val = val->val.b;

            // get the relay pin of the endpoint that must be updated
            light_handle_t * light_handle_ptr = (light_handle_t *)(driver_handle);
            uint8_t relay_pin = light_handle_ptr->relay_pin;

            // relay lvl = onoff attr val ; set relay gpio pin to relay lvl

            relay_set(relay_pin, onoff_val);

        }
    }



    return err;
}


