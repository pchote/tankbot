##*****************************************************************************
##  This file is part of tankbot, which is free software. It is made available
##  to you under version 3 (or later) of the GNU General Public License, as
##  published by the Free Software Foundation and included in the LICENSE file.
##*****************************************************************************

PORT := /dev/tty.usbmodem*

DEVICE = atmega2560
F_CPU = 16000000UL
AVRDUDE = avrdude -c stk500 -P $(PORT) -p $(DEVICE)
OBJECTS = main.o serial.o motor.o
COMPILE = avr-gcc -g -mmcu=$(DEVICE) -Wall -Wextra -Werror -Os -std=gnu99 -funsigned-bitfields -fshort-enums -DF_CPU=$(F_CPU)

all: main.hex reset

reset:
	$(CC) -o $@ reset.c

install: main.hex reset
	TERM=vt100 ./reset $(PORT)
	$(AVRDUDE) -U flash:w:main.hex:i

clean:
	rm -f main.hex main.elf reset $(OBJECTS)

disasm:	main.elf
	avr-objdump -d main.elf

size: main.elf
	avr-size -C --mcu=$(DEVICE) main.elf

main.elf: $(OBJECTS)
	$(COMPILE) -o main.elf $(OBJECTS)

main.hex: main.elf
	rm -f main.hex
	avr-objcopy -j .text -j .data -O ihex main.elf main.hex

.c.o:
	$(COMPILE) -c $< -o $@
