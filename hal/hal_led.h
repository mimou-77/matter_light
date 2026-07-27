
#pragma once


#ifdef __cplusplus
extern "C" {
#endif


#include "driver/gpio.h"


/*-----------------------------------------------------------------------------------------------*/
/* types                                                                                         */
/*-----------------------------------------------------------------------------------------------*/

// the led is wired active-LOW (anode to 3V3, cathode to the gpio) : driving 0 turns it ON
#define LED_ON_LVL 0
#define LED_OFF_LVL 1


/*-----------------------------------------------------------------------------------------------*/
/* headers                                                                                       */
/*-----------------------------------------------------------------------------------------------*/

void led_set(uint8_t pin, bool lvl);



/// @brief config led_gpio, then blink it
/// @param pin
void led_init(uint8_t pin);


/// @brief set_on, wait duration_ms, set_off
/// @param pin
/// @param duration_ms
void blink_led_for_duration_ms(uint8_t pin, uint16_t duration_ms);


/// @brief start toggling the led every interval_ms (non-blocking) ; keeps blinking until led_stop_blink() is called
/// @param pin
/// @param interval_ms time the led stays on / off between each toggle (visual blink period = 2 * interval_ms)
void led_start_blink(uint8_t pin, uint32_t interval_ms);


/// @brief stop the blink started by led_start_blink() and turn the led off
/// @param pin
void led_stop_blink(uint8_t pin);



#ifdef __cplusplus
}
#endif