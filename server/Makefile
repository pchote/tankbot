CC       = gcc
CFLAGS   = -g -Wall -Wno-unknown-pragmas --std=c99
LFLAGS   = -lpthread -lm
OBJS     = main.o avr.o serial.o

all: server

server: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LFLAGS)

clean:
	-rm $(OBJS) server

%.o : %.c
	$(CC) -c $(CFLAGS) $<
