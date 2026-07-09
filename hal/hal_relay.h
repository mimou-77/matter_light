
#pragma once
#ifndef __HAL_RELAY_H__
#define __HAL_RELAY_H__


#ifdef __cplusplus
extern "C" {
#endif


#include "driver/gpio.h"


/*-----------------------------------------------------------------------------------------------*/
/* types                                                                                         */
/*-----------------------------------------------------------------------------------------------*/




/*-----------------------------------------------------------------------------------------------*/
/* headers                                                                                       */
/*-----------------------------------------------------------------------------------------------*/

void relay_set(uint8_t pin, bool lvl);
void relay_toggle(uint8_t pin);


/// @brief config relay_gpio, then set it to 0
/// @param pin 
void relay_init(uint8_t pin);











#ifdef __cplusplus
}
#endif


#endif // __HAL_RELAY_H__