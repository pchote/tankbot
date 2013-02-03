//*****************************************************************************
//  ArduPilot communication thread
//
//  Copyright: 2013 Paul Chote
//  This file is part of tankbot, which is free software. It is made available
//  to you under version 3 (or later) of the GNU General Public License, as
//  published by the Free Software Foundation and included in the LICENSE file.
//*****************************************************************************

#ifndef TANKBOT_AVR_H
#define TANKBOT_AVR_H

#include <stdbool.h>
#include <stdint.h>

struct avr *avr_new(const char *port, uint32_t baud);
void avr_free(struct avr *avr);
void avr_shutdown(struct avr *avr);
bool avr_thread_alive(struct avr *avr);
void avr_set_speed(struct avr *avr, double left, double right);

#endif
