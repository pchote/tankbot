//*****************************************************************************
//  Shared protocol definitions for ArduPilot and server communication
//
//  Copyright: 2013 Paul Chote
//  This file is part of tankbot, which is free software. It is made available
//  to you under version 3 (or later) of the GNU General Public License, as
//  published by the Free Software Foundation and included in the LICENSE file.
//*****************************************************************************

#ifndef TANKBOT_PROTOCOL_H
#define TANKBOT_PROTOCOL_H

#include <stdint.h>

enum packet_type
{
    MESSAGE = 'M',
    SPEED = 'S',
    UNKNOWN = '0'
};

struct __attribute__((__packed__)) packet_speed
{
    uint16_t left;
    uint16_t right;
};

#define MAX_MESSAGE_LENGTH 200
struct __attribute__((__packed__)) packet_debug
{
    char message[1];
};

union packet_data
{
    struct packet_speed speed;
    struct packet_debug debug;

    // Ensure a suitable minimum length for debug messages
    uint8_t bytes[MAX_MESSAGE_LENGTH];
};

#endif
