//*****************************************************************************
//  Provides a generic interface to a hardware uart
//
//  Copyright: 2012, 2013 Paul Chote
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

void _serial_initialize(struct serial_port *port, uint32_t baud,
                         volatile uint8_t *ubrrh, volatile uint8_t *ubrrl,
                         volatile uint8_t *ucsra, volatile uint8_t *ucsrb,
                         volatile uint8_t *udr, const uint8_t u2x_val,
                         const uint8_t ucsrb_val, const uint8_t tx_enable_val)
{
    // Set requested baud rate
    uint16_t ubrr = (F_CPU / 4 / baud - 1) / 2;
    *ucsra = u2x_val;
    *ubrrh = ubrr >> 8;
    *ubrrl = ubrr;

    *ucsrb = ucsrb_val;
    port->input_read = port->input_write = 0;
    port->output_read = port->output_write = 0;

    port->udr = udr;
    port->ucsrb = ucsrb;
    port->tx_enable_val = tx_enable_val;
}

void serial_rx_handler(struct serial_port *port)
{
    port->input_buffer[(uint8_t)(port->input_write++)] = *port->udr;
}

void serial_tx_handler(struct serial_port *port)
{
    if (port->output_write != port->output_read)
        *port->udr = port->output_buffer[port->output_read++];

    // Ran out of data to send - disable the interrupt
    if (port->output_write == port->output_read)
        *port->ucsrb &= ~port->tx_enable_val;
}

uint8_t serial_bytes_available(struct serial_port *port)
{
    return port->input_write - port->input_read;
}

uint8_t serial_read_byte(struct serial_port *port)
{
    // Loop until data is available
    while (port->input_read == port->input_write);
    return port->input_buffer[port->input_read++];
}

void serial_write_byte(struct serial_port *port, uint8_t b)
{
    // Don't overwrite data that hasn't been sent yet
    while (port->output_write == (uint8_t)(port->output_read - 1));
    port->output_buffer[port->output_write++] = b;

    // Enable Transmit data register empty interrupt if necessary
    *port->ucsrb |= port->tx_enable_val;
}

void serial_write_bytes(struct serial_port *port, uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; i++)
        serial_write_byte(port, data[i]);
}

void serial_write_string(struct serial_port *port, const char *str)
{
    size_t len = strlen(str);
    for (size_t i = 0; i < len; i++)
        serial_write_byte(port, str[i]);
}

void serial_write_string_P(struct serial_port *port, const char *str)
{
    size_t len = strlen_P(str);
    for (size_t i = 0; i < len; i++)
        serial_write_byte(port, pgm_read_byte(&str[i]));
}

void serial_write_fmt(struct serial_port *port, const char *fmt, ...)
{
    va_list args;
    char buf[256];

    va_start(args, fmt);
    size_t len = vsnprintf(buf, 256, fmt, args);
    va_end(args);

    if (len > 128) len = 128;
    serial_write_bytes(port, (uint8_t *)buf, len);
}

void serial_write_fmt_P(struct serial_port *port, const char *fmt, ...)
{
    va_list args;
    char buf[256];

    va_start(args, fmt);
    size_t len = vsnprintf_P(buf, 256, fmt, args);
    va_end(args);

    if (len > 128) len = 128;
    serial_write_bytes(port, (uint8_t *)buf, len);
}
