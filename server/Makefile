#Simple make file for aesdsocket
CC = gcc
CFLAGS= -Wall -Werror
LDFLAGS=
CROSS_COMPILE=

EXECUTABLE=aesdsocket
SOURCES=aesdsocket.c
OBJECTS=$(SOURCES:.c=.o)

all: $(EXECUTABLE) $(OBJECTS)

$(EXECUTABLE): $(OBJECTS)
	$(CROSS_COMPILE)$(CC) $(LDFLAGS) $(OBJECTS) -o $@

$(OBJECTS): $(SOURCES)
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -c $<


clean:
	rm -f $(EXECUTABLE) $(OBJECTS)

