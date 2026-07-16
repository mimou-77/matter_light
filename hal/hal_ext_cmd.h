
#pragma once
#ifndef __HAL_EXT_CMD_H__
#define __HAL_EXT_CMD_H__


#ifdef __cplusplus
extern "C" {
#endif


/**
 * external cmd input : ext push btn switches 220Vac -> optocoupler (open collector) -> MCU pin
 * active LOW : idle = 1 (pull-up), pressed = pulled to 0 (pulsing at 50Hz while held)
 */


#include "driver/gpio.h"


/*-----------------------------------------------------------------------------------------------*/
/* types                                                                                         */
/*-----------------------------------------------------------------------------------------------*/

typedef void (*ext_cmd_isr_t)(void * arg);


/*-----------------------------------------------------------------------------------------------*/
/* headers                                                                                       */
/*-----------------------------------------------------------------------------------------------*/

/// @brief read the ext cmd pin level
bool ext_cmd_get(uint8_t pin);


/// @brief config the pin : input, pull-up, isr on any edge
void ext_cmd_init(uint8_t pin, ext_cmd_isr_t isr);




#ifdef __cplusplus
}
#endif


#endif // __HAL_EXT_CMD_H__
