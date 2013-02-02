//*****************************************************************************
//  Provides a generic interface to a hardware uart
//
//  Copyright: 2012, 2013 Paul Chote
//  This file is part of tankbot, which is free software. It is made available
//  to you under version 3 (or later) of the GNU General Public License, as
//  published by the Free Software Foundation and included in the LICENSE file.
//*****************************************************************************

#ifndef TANKBOT_SERIAL_H
#define TANKBOT_SERIAL_H

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <avr/interrupt.h>
void serial_initialize();
void serial_tick();
void serial_message_P(const char *string);
void serial_message_fmt_P(const char *fmt, ...);

#endif

