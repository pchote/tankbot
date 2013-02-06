//*****************************************************************************
//  Communication interface with iPhone using uart0 (USB / telemetry output)
//
//  Copyright: 2013 Paul Chote
//  This file is part of tankbot, which is free software. It is made available
//  to you under version 3 (or later) of the GNU General Public License, as
//  published by the Free Software Foundation and included in the LICENSE file.
//*****************************************************************************

#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <avr/pgmspace.h>
#include <string.h>
#include "serial.h"
#include "motor.h"
#include "../protocol.h"

const char unknown_packet_fmt[]  PROGMEM = "Unknown packet type '%c' - ignoring";
const char long_packet_fmt[]     PROGMEM = "Ignoring long packet: %c (length %u)";
const char checksum_failed_fmt[] PROGMEM = "Packet checksum failed. Got 0x%02x, expected 0x%02x";
const char invalid_packet_fmt[] PROGMEM = "Invalid packet end byte. Got 0x%02x, expected 0x%02x";

static uint8_t input_buffer[256];
static uint8_t input_read = 0;
static volatile uint8_t input_write = 0;

static uint8_t output_buffer[256];
static volatile uint8_t output_read = 0;
static volatile uint8_t output_write = 0;

// Add a byte to the send buffer.
// Will block if the buffer is full
static void queue_byte(uint8_t b)
{
    // Don't overwrite data that hasn't been sent yet
    while (output_write == (uint8_t)(output_read - 1));

    output_buffer[output_write++] = b;

    // Enable transmit if necessary
    UCSR0B |= _BV(UDRIE0);
}

// Send data from RAM
static void queue_data(uint8_t type, const void *data, uint8_t length)
{
    // Header
    queue_byte('$');
    queue_byte('$');
    queue_byte(type);
    queue_byte(length);

    // Data
    uint8_t checksum = 0;
    for (uint8_t i = 0; i < length; i++)
    {
        uint8_t b = ((uint8_t *)data)[i];
        queue_byte(b);
        checksum ^= b;
    }

    // Footer
    queue_byte(checksum);
    queue_byte('\r');
    queue_byte('\n');
}

// Send data from flash
static void queue_data_P(uint8_t type, const void *data, uint8_t length)
{
    // Data packet starts with $$ and packet type (which != $)
    queue_byte('$');
    queue_byte('$');
    queue_byte(type);

    // Length of data section
    queue_byte(length);

    // Packet data - calculate checksum as we go
    uint8_t csm = 0;
    for (uint8_t i = 0; i < length; i++)
    {
        uint8_t b = pgm_read_byte(&((uint8_t *)data)[i]);
        queue_byte(b);
        csm ^= b;
    }

    // Checksum
    queue_byte(csm);

    // Data packet ends a linefeed and carriage return
    queue_byte('\r');
    queue_byte('\n');
}

static bool byte_available()
{
    return input_write != input_read;
}

static uint8_t read_byte()
{
    // Loop until data is available
    while (input_read == input_write);
    return input_buffer[input_read++];
}

ISR(USART0_UDRE_vect)
{
    if(output_write != output_read)
        UDR0 = output_buffer[output_read++];

    // Ran out of data to send - disable the interrupt
    if(output_write == output_read)
        UCSR0B &= ~_BV(UDRIE0);
}

ISR(USART0_RX_vect)
{
    input_buffer[(uint8_t)(input_write++)] = UDR0;
}

void serial_initialize()
{
    // Set baud rate to 115.2k
    UBRR0H = 0;
    UBRR0L = 16;
    UCSR0A = _BV(U2X0);

    // Enable receive, transmit, data received interrupt
    UCSR0B = _BV(RXEN0) | _BV(TXEN0) | _BV(RXCIE0);

    input_read = input_write = 0;
    output_read = output_write = 0;
}

static void parse_packet(enum packet_type type, union packet_data data)
{
    switch (type)
    {
    case SPEED:
        motor_set_speeds(data.speed.left, data.speed.right);
        break;
    default:
        serial_message_fmt_P(unknown_packet_fmt, type);
    }
}

/*
 * Process any data in the received buffer
 * Parses at most one packet - so must be called frequently
 */
void serial_tick()
{
    // Parsing state
    static uint8_t state = 0;
    static enum packet_type type;
    static uint8_t checksum = 0;
    static uint8_t length = 0;
    static uint8_t read = 0;
    static union packet_data data;

    while (byte_available())
    {
        uint8_t b = read_byte();
        switch (state)
        {
        case 0: // Frame start characters
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
                serial_message_fmt_P(long_packet_fmt, type, length);
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
                serial_message_fmt_P(checksum_failed_fmt, b, checksum);
                state = 0;
            }
            break;
        case 6: // Frame end bytes
            if (b == '\r')
                state++;
            else
            {
                serial_message_fmt_P(invalid_packet_fmt, b, '\r');
                state = 0;
            }
            break;
        case 7:
            if (b == '\n')
                parse_packet(type, data);
            else
                serial_message_fmt_P(invalid_packet_fmt, b, '\n');
            state = 0;
            break;
        }
    }
}

void serial_message_P(const char *string)
{
    size_t len = strlen_P(string);
    if (len > MAX_MESSAGE_LENGTH)
        len = MAX_MESSAGE_LENGTH;

    queue_data_P(MESSAGE, string, len);
}

void serial_message_fmt_P(const char *fmt, ...)
{
    va_list args;
    char buf[MAX_MESSAGE_LENGTH];

    va_start(args, fmt);
    int len = vsnprintf_P(buf, MAX_MESSAGE_LENGTH, fmt, args);
    va_end(args);

    if (len > MAX_MESSAGE_LENGTH)
        len = MAX_MESSAGE_LENGTH;
    queue_data(MESSAGE, buf, (uint8_t)len);
}
