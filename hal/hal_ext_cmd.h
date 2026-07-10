
#pragma once
#ifndef __HAL_EXT_CMD_H__
#define __HAL_EXT_CMD_H__


#ifdef __cplusplus
extern "C" {
#endif




/**
 * @brief external cmd = a transistor  
 * 
 * 
 */





#include "driver/gpio.h"


/*-----------------------------------------------------------------------------------------------*/
/* types                                                                                         */
/*-----------------------------------------------------------------------------------------------*/

typedef void (*ext_cmd_isr_t)(void * arg);


/*-----------------------------------------------------------------------------------------------*/
/* headers                                                                                       */
/*-----------------------------------------------------------------------------------------------*/

bool ext_cmd_get(uint8_t pin);


/// @brief config an external command input pin (active HIGH : the external transistor drives
///        the pin to 1 while the external push btn is pressed) ; pull-down keeps it at 0 when idle
/// @param pin
/// @param isr called on any edge ; pin nbr is passed as arg
void ext_cmd_init(uint8_t pin, ext_cmd_isr_t isr);




#ifdef __cplusplus
}
#endif


#endif // __HAL_EXT_CMD_H__
