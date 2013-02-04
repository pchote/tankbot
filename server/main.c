//*****************************************************************************
//  Copyright: 2012, 2013 Paul Chote
//  This file is part of tankbot, which is free software. It is made available
//  to you under version 3 (or later) of the GNU General Public License, as
//  published by the Free Software Foundation and included in the LICENSE file.
//*****************************************************************************

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include "avr.h"
#include "serial.h"

int main(int argc, char *argv[])
{
    struct avr *avr = avr_new("/dev/tty.iap", 115200);
    if (!avr)
    {
        printf("Failed to initialize AVR connection\n");
        return 1;
    }

    while (avr_thread_alive(avr))
    {
        sleep(4);
        avr_set_speed(avr, 0.2, 0.3);
        avr_set_speed(avr, 0.2, 0.3);
        sleep(3);
        avr_set_speed(avr, 0.0, 0.4);
        sleep(3);
        avr_set_speed(avr, 0.3, 0.3);
        sleep(3);
        avr_set_speed(avr, 0.2, 0.0);

        // Sleep for 100ms
        nanosleep(&(struct timespec){0, 1e8}, NULL);
    }

    avr_free(avr);
    return 0;
}