
#pragma once
#ifndef __HAL_DEFS_H__
#define __HAL_DEFS_H__


#define LED_PIN 9
#define LED_ADV_BLINK_INTERVAL_MS 100 // fast blink while advertising ; visual period = 200ms (~5Hz)

// endpoint 1
#define PUSH_BTN_1_PIN 3
#define RELAY_1_PIN 0
#define EXT_CMD_1_PIN 19 // external push btn 1 : transistor toggles relay 1 in hw and drives this pin to 1

// endpoint 2
#define PUSH_BTN_2_PIN 23
#define RELAY_2_PIN 18
#define EXT_CMD_2_PIN 20 // external push btn 2 : transistor toggles relay 2 in hw and drives this pin to 1




/*-----------------------------------------------------------------------------------------------*/
/* types                                                                                         */
/*-----------------------------------------------------------------------------------------------*/




/*-----------------------------------------------------------------------------------------------*/
/* headers                                                                                       */
/*-----------------------------------------------------------------------------------------------*/










#endif // __HAL_DEFS_H__