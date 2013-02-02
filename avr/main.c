//*****************************************************************************
//  Copyright: 2012, 2013 Paul Chote
//  This file is part of tankbot, which is free software. It is made available
//  to you under version 3 (or later) of the GNU General Public License, as
//  published by the Free Software Foundation and included in the LICENSE file.
//*****************************************************************************

#include <avr/pgmspace.h>
#include <util/delay.h>
#include "serial.h"
#include "motor.h"

const char debug_startup_complete[] PROGMEM = "Startup complete";

int main(void)
{
    serial_initialize();
    motor_initialize();

    // TODO: Disable unused hardware to save power

    sei();
    serial_message_P(debug_startup_complete);
    for (;;)
        serial_tick();
}
