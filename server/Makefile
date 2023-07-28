#Simple make file for aesdsocket
#CC = gcc
CFLAGS= -g -Wall -Werror
LDFLAGS= -pthread -lrt
#CROSS_COMPILE=aarch64-none-linux-gnu
CROSS_COMPILE?= aarch64-none-linux-gnu
CC?= $(CROSS_COMPILE)gcc
EXECUTABLE=aesdsocket
SOURCES=aesdsocket.c
OBJECTS=$(SOURCES:.c=.o)

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(INCLUDES) $(OBJECTS) -o $(EXECUTABLE) $(LDFLAGS) $(CFLAGS)


clean:
	rm -f $(EXECUTABLE) $(OBJECTS)

