
#pragma once
#ifndef __LIGHT_DRIVER_MATTER_H__
#define __LIGHT_DRIVER_MATTER_H__


#include "hal_relay.h"
#include "hal_push_btn.h"
#include "hal_ext_cmd.h"
#include "matter_defs.h"


#include <esp_matter.h>
#include <esp_matter_cluster.h>
#include <esp_matter_endpoint.h>

#include "freertos/FreeRTOS.h"

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
    uint8_t relay_pin;
    uint8_t push_btn_pin;
    uint8_t ext_cmd_pin;

} light_handle_t;



typedef struct
{
    uint8_t pin;
    int level;      // 0 : pressed ; 1 : released
    TickType_t timestamp; 
} btn_event_t;


typedef struct
{
    uint8_t pin;
    light_handle_t * light;  // light (relay_pin + endpoint_id) controlled by this button
    TickType_t press_time;   // time of the last accepted press edge

    // debounced state (managed by push_btn_task ; leave zero-initialized)
    bool pressed;            // stable level : true = currently pressed
    bool in_combo;           // this press is part of a both-btns-pressed combo => no toggle on release
    TickType_t last_edge;    // time of the last ACCEPTED edge (bounce filter reference)

} btn_ctx_t;


typedef struct
{
    uint8_t pin;
    light_handle_t * light;    // light toggled by this ext cmd

    TickType_t last_event;     // time of the last edge (managed by push_btn_task ; leave zero-initialized)

} ext_cmd_ctx_t;


typedef struct
{
    btn_ctx_t btn_1;
    btn_ctx_t btn_2;

    ext_cmd_ctx_t ext_1;
    ext_cmd_ctx_t ext_2;

} light_ctx_t;


/*-----------------------------------------------------------------------------------------------*/
/* headers                                                                                       */
/*-----------------------------------------------------------------------------------------------*/


/// @brief - config gpio for the light and store pins the handle
///        - create endpoint attached to node ; store endpoint_id in the handle ; (endpoint type is onoff light)
///        - create onoff cluster, onoff attribute
/// @param relay_pin
/// @param push_btn_pin
/// @param ext_cmd_pin
/// @return
extern "C" light_handle_t * create_light(node_t * light_node, uint8_t relay_pin, uint8_t push_btn_pin, uint8_t ext_cmd_pin);


extern "C" void push_btn_isr(void *arg);


extern "C" void push_btn_task(void *pvParameters);

void fctry_reset_timer_cb(void * args);


#endif // __LIGHT_DRIVER_MATTER_H__