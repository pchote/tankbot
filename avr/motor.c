//*****************************************************************************
//  Controls motor speed by outputting PWM signals to the ESCs using Timer1.
//
//  Copyright: 2012, 2013 Paul Chote
//  This file is part of tankbot, which is free software. It is made available
//  to you under version 3 (or later) of the GNU General Public License, as
//  published by the Free Software Foundation and included in the LICENSE file.
//*****************************************************************************

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "motor.h"
#include "serial.h"

const char debug_speed_fmt[] PROGMEM = "Speed set to %u%%, %u%%";

#define SPEED_MIN_LEFT 0U
#define SPEED_MAX_LEFT 65535U

#define SPEED_MIN_RIGHT 0U
#define SPEED_MAX_RIGHT 65535U

#define ABS(x) ((x) > 0 ? (x) : (-x))
#define SIGN(x) ((x) > 0 ? 1 : -1)
#define CLAMP(x, min, max) (((x) >= (max)) ? (max) : (((x) <= (min)) ? (min) : (x)))
#define MAP_SPEED_LEFT(x) (uint16_t)(SPEED_MIN_LEFT + ((uint32_t)(x)*(SPEED_MAX_LEFT - SPEED_MIN_LEFT) / 10000))
#define MAP_SPEED_RIGHT(x) (uint16_t)(SPEED_MIN_RIGHT + ((uint32_t)(x)*(SPEED_MAX_RIGHT - SPEED_MIN_RIGHT) / 10000))

void motor_initialize()
{    
    // 16-bit fast PWM @ 50Hz
    TCCR1A = _BV(COM1A1) | _BV(COM1B1) | _BV(WGM11);
    TCCR1B = _BV(WGM13) | _BV(WGM12) | _BV(CS11);
    ICR1 = 39999;
    
    OCR1A = 0;
    OCR1B = 0;
    DDRB |= _BV(PB5) | _BV(PB6);
    DDRE |= _BV(PE3) | _BV(PE4) | _BV(PE5);
    DDRH |= _BV(PH3);
    _delay_ms(5000);
}

// Left motor on ArduPilot Mega output channel 1 (PB6)
// Right motor on ArduPilot Mega output channel 2 (PB5)
// left and right use 0 - 10000 to represent fixed point values 0 - 1.0000
void motor_set_speeds(int16_t left, int16_t right)
{
    if (SIGN(left))
    {
        // Forwards
        PORTE &= ~_BV(PE3);
        PORTE |= _BV(PE4);
    }
    else
    {
        PORTE |= _BV(PE3);
        PORTE &= ~_BV(PE4);
    }
    left = CLAMP(ABS(left), 0, 10000);
    OCR1B = MAP_SPEED_LEFT(left);

    if (SIGN(right))
    {
        // Forwards
        PORTH |= _BV(PH3);
        PORTE &= ~_BV(PE5);
    }
    else
    {
        PORTH &= ~_BV(PH3);
        PORTE |= _BV(PE5);
    }
    right = CLAMP(ABS(right), 0, 10000);
    OCR1A = MAP_SPEED_RIGHT(right);

    serial_message_fmt_P(debug_speed_fmt, (uint8_t)(left / 100), (uint8_t)(right / 100));
}
