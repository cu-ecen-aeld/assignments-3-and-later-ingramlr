#Simple make file for aesdsocket
CC?=$(CROSS_COMPILE)gcc
CFLAGS?=-Wall -Werror -g -O0
LDFLAGS?=-lrt -pthread

TARGET?=aesdsocket


all: $(TARGET)


$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET).o
	rm -f $(TARGET)

