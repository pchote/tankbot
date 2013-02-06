//*****************************************************************************
//  Webserver implemented using libwebsocket
//
//  Copyright: 2013 Paul Chote
//  This file is part of tankbot, which is free software. It is made available
//  to you under version 3 (or later) of the GNU General Public License, as
//  published by the Free Software Foundation and included in the LICENSE file.
//*****************************************************************************

#ifndef TANKBOT_WEBSERVER_H
#define TANKBOT_WEBSERVER_H

struct webserver;

struct webserver *webserver_create(int port);
void webserver_free(struct webserver *webserver);
int webserver_tick(struct webserver *webserver, int timeout_ms);
void webserver_send_debug(struct webserver *webserver, const char *message, size_t length);

#endif




