
#pragma once
#ifndef __HAL_PUSH_BTN_H__
#define __HAL_PUSH_BTN_H__


#ifdef __cplusplus
extern "C" {
#endif



#include "driver/gpio.h"


/*-----------------------------------------------------------------------------------------------*/
/* types                                                                                         */
/*-----------------------------------------------------------------------------------------------*/

typedef void (*push_btn_isr_t)(void * arg);


/*-----------------------------------------------------------------------------------------------*/
/* headers                                                                                       */
/*-----------------------------------------------------------------------------------------------*/

bool push_btn_get(uint8_t pin);


void push_btn_init(uint8_t pin, push_btn_isr_t isr);




#ifdef __cplusplus
}
#endif


#endif // __HAL_PUSH_BTN_H__