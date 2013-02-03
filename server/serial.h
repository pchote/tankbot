//*****************************************************************************
//  Simple serial port interface
//
//  Copyright: 2013 Paul Chote
//  This file is part of tankbot, which is free software. It is made available
//  to you under version 3 (or later) of the GNU General Public License, as
//  published by the Free Software Foundation and included in the LICENSE file.
//*****************************************************************************

#ifndef TANKBOT_SERIAL_H
#define TANKBOT_SERIAL_H

#include <stdbool.h>
#include <stdint.h>

struct serial_port;
struct serial_port *serial_port_open(const char *path, uint32_t baud);
void serial_port_close(struct serial_port *port);
ssize_t serial_port_read(struct serial_port *port, uint8_t *buf, size_t length);
ssize_t serial_port_write(struct serial_port *port, const uint8_t *buf, size_t length);
const char *serial_port_error_string(struct serial_port *port, ssize_t code);

#endif
