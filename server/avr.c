//*****************************************************************************
//  ArduPilot communication thread
//
//  Copyright: 2013 Paul Chote
//  This file is part of tankbot, which is free software. It is made available
//  to you under version 3 (or later) of the GNU General Public License, as
//  published by the Free Software Foundation and included in the LICENSE file.
//*****************************************************************************

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "avr.h"
#include "serial.h"
#include "webserver.h"
#include "../protocol.h"

extern struct webserver *webserver;
struct avr
{
    pthread_t thread;
    bool thread_alive;
    bool shutdown;

    char *serial_port;
    uint32_t serial_baud;

    uint8_t send_buffer[256];
    uint8_t send_length;
    pthread_mutex_t send_mutex;
};

static void *avr_thread(void *avr);

struct avr *avr_new(const char *port, uint32_t baud)
{
    struct avr *avr = calloc(1, sizeof(struct avr));
    if (!avr)
        return NULL;

    avr->serial_port = strdup(port);
    avr->serial_baud = baud;
    pthread_mutex_init(&avr->send_mutex, NULL);

    // Spawn the worker thread
    avr->thread_alive = true;
    if (pthread_create(&avr->thread, NULL, avr_thread, avr))
    {
        printf("Failed to spawn avr communication thread\n");
        avr_free(avr);
        return NULL;
    }

    return avr;
}

void avr_free(struct avr *avr)
{
    pthread_mutex_destroy(&avr->send_mutex);
    free(avr->serial_port);
    free(avr);
}

// Queue a raw byte to be sent to the avr
// Should only be called by queue_data
static void queue_send_byte(struct avr *avr, uint8_t b)
{
    // Hard-loop until the send buffer empties
    // Should never happen in normal operation
    // TODO: This should use a signal or return an error code
    while (avr->send_length >= 255);

    pthread_mutex_lock(&avr->send_mutex);
    avr->send_buffer[avr->send_length++] = b;
    pthread_mutex_unlock(&avr->send_mutex);
}

// Wrap an array of bytes in a data packet and send it to the timer
static void queue_data(struct avr *avr, enum packet_type type, void *_data, size_t length)
{
    uint8_t *data = _data;

    // Header
    queue_send_byte(avr, '$');
    queue_send_byte(avr, '$');
    queue_send_byte(avr, type);
    queue_send_byte(avr, length);

    // Data
    uint8_t checksum = 0;
    for (uint8_t i = 0; i < length; i++)
    {
        checksum ^= data[i];
        queue_send_byte(avr, data[i]);
    }

    // Footer
    queue_send_byte(avr, checksum);
    queue_send_byte(avr, '\r');
    queue_send_byte(avr, '\n');
}

static void parse_packet(struct avr *avr, enum packet_type type, union packet_data data, uint8_t length)
{
    switch (type)
    {
    case MESSAGE:
        {
            printf("AVR Message: ");
            for (uint8_t i = 0; i < length; i++)
                putchar(data.debug.message[i]);

            // TODO: Relying on an extern global isn't great.
            webserver_send_debug(webserver, data.debug.message, length);
            printf("\n");
        }
        break;
    default:
        printf("Unknown packet type: %c\n", type);
    }
}

// Main timer thread loop
static void *avr_thread(void *_avr)
{
    struct avr *avr = (struct avr *)_avr;

    // Attempt to open the serial connection
    struct serial_port *port = serial_port_open(avr->serial_port, avr->serial_baud);
    if (!port)
    {
        printf("Failed to open serial port\n");
        goto error;
    }

    // Parsing state
    uint8_t state = 0;
    enum packet_type type;
    uint8_t checksum = 0;
    uint8_t length = 0;
    uint8_t read = 0;
    static union packet_data data;

    // Loop until shutdown, parsing incoming data
    while (!avr->shutdown)
    {
        // Send any queued data
        pthread_mutex_lock(&avr->send_mutex);
        if (avr->send_length > 0)
        {
            ssize_t ret = serial_port_write(port, avr->send_buffer, avr->send_length);
            if (ret < 0)
            {
                printf("Write error %zd: %s", ret, serial_port_error_string(port, ret));
                pthread_mutex_unlock(&avr->send_mutex);
                break;
            }

            if (ret != avr->send_length)
            {
                printf("Incomplete write: only %zu of %u bytes sent\n", ret, avr->send_length);                
                pthread_mutex_unlock(&avr->send_mutex);
                break;
            }
            avr->send_length = 0;
        }
        pthread_mutex_unlock(&avr->send_mutex);

        // Check for new data
        uint8_t b;
        ssize_t r;
        while ((r = serial_port_read(port, &b, 1)) > 0)
        {
            switch (state)
            {
            case 0: // Synchronize to packet header
            case 1:
                if (b == '$')
                    state++;
                else
                    state = 0;
                break;
            case 2: // Packet type
                type = b;
                state++;
                break;
            case 3: // Message length
                length = b;
                read = 0;
                checksum = 0;
                if (length <= sizeof(data))
                    state++;
                else
                {
                    printf("Ignoring long packet: %c (length %u)\n", type, length);
                    state = 0;
                }
                break;
            case 4: // Message data
                checksum ^= b;
                data.bytes[read++] = b;
                if (read == length)
                    state++;
                break;
            case 5: // checksum byte
                if (checksum == b)
                    state++;
                else
                {
                    printf("Packet checksum failed. Got 0x%02x, expected 0x%02x.\n", b, checksum);
                    state = 0;
                }
                break;
            case 6: // Packet Footer
                if (b == '\r')
                    state++;
                else
                {
                    printf("Invalid packet end byte. Got 0x%02x, expected 0x%02x.\n", b, '\r');
                    state = 0;
                }
                break;
            case 7:
                if (b == '\n')
                    parse_packet(avr, type, data, length);
                else
                    printf("Invalid packet end byte. Got 0x%02x, expected 0x%02x.\n", b, '\n');

                state = 0;
                break;
            }
        }

        if (r < 0)
        {
            printf("Read error %zd: %s\n", r, serial_port_error_string(port, r));
            break;
        }

        // Sleep for 100ms
        nanosleep(&(struct timespec){0, 1e8}, NULL);
    }

error:
    if (port)
        serial_port_close(port);
    avr->thread_alive = false;
    return NULL;
}

void avr_shutdown(struct avr *avr)
{
    avr->shutdown = true;
    void **retval = NULL;
    if (avr->thread_alive)
        pthread_join(avr->thread, retval);
}

bool avr_thread_alive(struct avr *avr)
{
    return avr->thread_alive;
}

void avr_set_speed(struct avr *avr, double left, double right)
{
    struct packet_speed speed = {
        .left = (uint16_t)(10000*left),
        .right = (uint16_t)(10000*right)
    };
    queue_data(avr, SPEED, &speed, sizeof(struct packet_speed));
}
