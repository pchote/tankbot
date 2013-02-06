//*****************************************************************************
//  Copyright: 2012, 2013 Paul Chote
//  This file is part of tankbot, which is free software. It is made available
//  to you under version 3 (or later) of the GNU General Public License, as
//  published by the Free Software Foundation and included in the LICENSE file.
//*****************************************************************************

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#include "avr.h"
#include "serial.h"
#include "webserver.h"


struct avr *avr;
struct webserver *webserver;

bool force_shutdown = false;
void shutdown_handler(int foo)
{
    force_shutdown = true;
}

int main(int argc, char *argv[])
{
    signal(SIGINT, shutdown_handler);

    avr = avr_new("/dev/tty.iap", 115200);
    if (!avr)
    {
        printf("Failed to initialize AVR connection\n");
        return 1;
    }

    webserver = webserver_create(7681);
    if (!webserver)
    {
        printf("Failed to initialize webserver\n");
        return 1;
    }

    int n = 0;
    while (n >= 0)
    {
        if (!avr_thread_alive(avr))
        {
            printf("avr thread died unexpectedly\n");
            break;
        }

        if (force_shutdown)
        {
            printf("Shutdown requested!\n");
            avr_shutdown(avr);
            break;
        }

        n = webserver_tick(webserver, 50);
    }

    webserver_free(webserver);

    avr_free(avr);
    printf("Exiting cleanly\n");
    return 0;
}