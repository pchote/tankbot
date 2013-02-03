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
#include "motor.h"
#include "serial.h"

const char debug_speed_fmt[] PROGMEM = "Speed set to %u%%, %u%%";

// PWM limits measured from a RC receiver
// TODO: Investigate whether these limits make the most of the ESC input range
#define SPEED_MIN 1760U
#define SPEED_MAX 3999U
#define CLAMP(x, min, max) (((x) >= (max)) ? (max) : (((x) <= (min)) ? (min) : (x)))
#define MAP_SPEED(x) (uint16_t)(SPEED_MIN + ((uint32_t)(x)*(SPEED_MAX - SPEED_MIN) / 10000))

void motor_initialize()
{    
    // 16-bit fast PWM @ 50Hz
    TCCR1A = _BV(COM1A1) | _BV(COM1B1) | _BV(WGM11);
    TCCR1B = _BV(WGM13) | _BV(WGM12) | _BV(CS11);
    ICR1 = 39999;

    // The ESCs require the minimum PWM level to be set before startup.
    // They will not operate if the duty cycle is less than the minimum level.
    OCR1A = SPEED_MIN;
    OCR1B = SPEED_MIN;
    DDRB |= _BV(PB5) | _BV(PB6);
}

// Left motor on ArduPilot Mega output channel 1 (PB6)
// Right motor on ArduPilot Mega output channel 2 (PB5)
// left and right use 0 - 10000 to represent fixed point values 0 - 1.0000
void motor_set_speeds(uint16_t left, uint16_t right)
{
    left = CLAMP(left, 0, 10000);
    right = CLAMP(right, 0, 10000);
    OCR1B = MAP_SPEED(left);
    OCR1A = MAP_SPEED(right);

    serial_message_fmt_P(debug_speed_fmt, (uint8_t)(left / 100), (uint8_t)(right / 100));
}
