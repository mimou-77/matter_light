
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


typedef enum
{
    EXT_CMD_ARMED = 0,   // line at rest : the next edge opens a pending actuation
    EXT_CMD_PENDING,     // 1 edge seen : fires on a 2nd edge or a non-idle level after
                         // EXT_CMD_CONFIRM_MS ; discarded if the line is back at idle (glitch)
    EXT_CMD_SETTLING,    // actuation fired : everything is absorbed until the line has been quiet
                         // for EXT_CMD_SETTLE_MS and is back at its idle level

} ext_cmd_state_t;


typedef struct
{
    uint8_t pin;
    light_handle_t * light;    // light (endpoint_id) whose relay is toggled in hw by this ext cmd

    // managed by push_btn_task ; leave zero-initialized
    // (all time references are PROCESSING time (xTaskGetTickCount at dequeue), never the isr
    //  timestamp : the logic stays correct even if events sat in the queue during a stall)
    ext_cmd_state_t state;
    bool idle_lvl;             // level of the line at rest (sampled once at task start)
    TickType_t last_event;     // time the last edge of this line was processed (quiet reference)
    TickType_t pending_since;  // time the pending 1st edge was processed (confirm deadline)
    TickType_t disarm_time;    // time the line entered SETTLING (idle-resync reference)

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