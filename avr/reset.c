//*****************************************************************************
//  Toggles the DTR output on the Arduino to reset into the bootloader
//
//  Copyright: 2013 Paul Chote
//  This file is part of tankbot, which is free software. It is made available
//  to you under version 3 (or later) of the GNU General Public License, as
//  published by the Free Software Foundation and included in the LICENSE file.
//*****************************************************************************

#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: reset <path to port>\n");
        return 1;
    }

    int port = open(argv[1], O_RDWR | O_NOCTTY | O_NDELAY);
    if (port == -1)
    {
        printf("Failed to open port: %s\n", argv[1]);
        return 1;
    }
    
    // Toggle DTR to trigger a hardware reset
    ioctl(port, TIOCMSET, &(int){TIOCM_DTR});
    sleep(1);
    ioctl(port, TIOCMSET, &(int){0});
    close(port);

    return 0;
}
