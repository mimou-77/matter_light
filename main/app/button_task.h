
#pragma once

#include "matter_light_endpoint.h"

#include "freertos/FreeRTOS.h"


/*-----------------------------------------------------------------------------------------------*/
/* types                                                                                         */
/*-----------------------------------------------------------------------------------------------*/

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
    btn_ctx_t btns[MAX_LIGHTS];
    ext_cmd_ctx_t exts[MAX_LIGHTS];

} light_ctx_t;


/*-----------------------------------------------------------------------------------------------*/
/* headers                                                                                       */
/*-----------------------------------------------------------------------------------------------*/

/// @brief lazily creates the shared btn-event queue (once), then configures push_btn_pin + ext_cmd_pin
///        as inputs with an any-edge ISR posting {pin, level, timestamp} to that queue.
///        call once per light, after create_light() and before push_btn_task is started.
/// @param push_btn_pin
/// @param ext_cmd_pin
void button_task_register_light_io(uint8_t push_btn_pin, uint8_t ext_cmd_pin);


/// @brief handles the local btns (press < 1s = toggle ; both held 10s = factory reset)
///        and the ext cmd lines (1 press = 1 onoff toggle)
///
///        btn debounce : an edge is accepted only if it changes the stable state of the btn
///        AND arrives at least DEBOUNCE_MS after the last accepted edge
///
/// @param pvParameters light_ctx_t *
extern "C" void push_btn_task(void *pvParameters);
