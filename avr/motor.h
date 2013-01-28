//*****************************************************************************
//  Controls motor speed by outputting PWM signals to the ESCs using Timer1.
//
//  Copyright: 2012, 2013 Paul Chote
//  This file is part of tankbot, which is free software. It is made available
//  to you under version 3 (or later) of the GNU General Public License, as
//  published by the Free Software Foundation and included in the LICENSE file.
//*****************************************************************************

#ifndef TANKBOT_MOTOR_H
#define TANKBOT_MOTOR_H

void motor_initialize();
void motor_set_left_speed(double fraction);
void motor_set_right_speed(double fraction);

#endif
