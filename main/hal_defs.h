
#pragma once
#ifndef __HAL_DEFS_H__
#define __HAL_DEFS_H__


#define LED_PIN 9
#define LED_ADV_BLINK_INTERVAL_MS 100 // fast blink while advertising ; visual period = 200ms (~5Hz)

// endpoint 1
#define PUSH_BTN_1_PIN 3
#define RELAY_1_PIN 0
#define EXT_CMD_1_PIN 19 // external push btn 1 input

// endpoint 2
#define PUSH_BTN_2_PIN 23
#define RELAY_2_PIN 18
#define EXT_CMD_2_PIN 20 // external push btn 2 input

// ext cmd chain : ext btn switches 220Vac -> 2x220k -> EL357 optocoupler (open collector) -> pin
// active LOW : idle = 1 (pull-up), pressed = pulled to 0 (pulsing at 50Hz while held)




/*-----------------------------------------------------------------------------------------------*/
/* types                                                                                         */
/*-----------------------------------------------------------------------------------------------*/




/*-----------------------------------------------------------------------------------------------*/
/* headers                                                                                       */
/*-----------------------------------------------------------------------------------------------*/










#endif // __HAL_DEFS_H__