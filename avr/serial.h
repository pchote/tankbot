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

struct serial_port
{
    uint8_t input_buffer[256];
    uint8_t input_read;
    volatile uint8_t input_write;

    uint8_t output_buffer[256];
    volatile uint8_t output_read;
    volatile uint8_t output_write;

    volatile uint8_t *ucsrb;
    volatile uint8_t *udr;
    uint8_t tx_enable_val;
};

#define serial_initialize(port, id, baud) \
_serial_initialize(port, baud,    \
                   &UBRR##id##H,  \
                   &UBRR##id##L,  \
                   &UCSR##id##A,  \
                   &UCSR##id##B,  \
                   &UDR##id,      \
                   _BV(U2X##id),  \
                   (_BV(RXEN##id) |  _BV(TXEN##id) | _BV(RXCIE##id)), \
                   (_BV(UDRIE##id)));

#define serial_insert_handlers(port, id) \
ISR(USART##id##_RX_vect) \
{ \
    serial_rx_handler(port); \
} \
ISR(USART##id##_UDRE_vect) \
{ \
    serial_tx_handler(port); \
}

void _serial_initialize(struct serial_port *port, uint32_t baud,
                        volatile uint8_t *ubrrh, volatile uint8_t *ubrrl,
                        volatile uint8_t *ucsra, volatile uint8_t *ucsrb,
                        volatile uint8_t *udr, const uint8_t u2x_val,
                        const uint8_t ucsrb_val, const uint8_t tx_enable_val);
void serial_rx_handler(struct serial_port *port);
void serial_tx_handler(struct serial_port *port);
uint8_t serial_bytes_available(struct serial_port *port);
uint8_t serial_read_byte(struct serial_port *port);
void serial_write_byte(struct serial_port *port, uint8_t b);
void serial_write_bytes(struct serial_port *port, uint8_t *data, size_t len);
void serial_write_string(struct serial_port *port, const char *str);
void serial_write_string_P(struct serial_port *port, const char *str);
void serial_write_fmt(struct serial_port *port, const char *fmt, ...);
void serial_write_fmt_P(struct serial_port *port, const char *fmt, ...);

#endif
