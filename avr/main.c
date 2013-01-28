//*****************************************************************************
//  Copyright: 2012, 2013 Paul Chote
//  This file is part of tankbot, which is free software. It is made available
//  to you under version 3 (or later) of the GNU General Public License, as
//  published by the Free Software Foundation and included in the LICENSE file.
//*****************************************************************************

#include <stdint.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

#include "serial.h"
#include "motor.h"

const char debug_speed_fmt[] PROGMEM = "Speed set to %u%%\r\n";

struct serial_port usb_uart;
serial_insert_handlers(&usb_uart, 0);

int main(void)
{
    struct serial_port *debug = &usb_uart;

    serial_initialize(debug, 0, 115200);
    motor_initialize();

    // TODO: Disable unused hardware to save power

    sei();

    // Test motors
    float speeds[] = {0.1, 0.5, 0.1, 0.0};
    uint8_t speed_count = 4;

    for (;;)
    {
        for (uint8_t i = 0; i < speed_count; i++)
        {
            _delay_ms(5000.0);
            motor_set_left_speed(speeds[i]);
            motor_set_right_speed(speeds[i]);
            serial_write_fmt_P(debug, debug_speed_fmt, (uint8_t)(100*speeds[i]));
        }
    }
}
