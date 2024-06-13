.PHONY: all clean
CC ?= gcc
CROSS_COMPILE ?= 
CFLAGS ?= -Wall -Werror
SRCS = aesdsocket.c
TARGET ?= aesdsocket
LDFLAGS ?= -pthread -lrt

$(TARGET): $(TARGET).o
	$(CROSS_COMPILE)$(CC) $(CFLAGS) $(LDFLAGS) -I/ $(TARGET).o -o $(TARGET)
##$(TARGET).o:$(TARGET).c
##	$(CROSS_COMPILE)$(CC) $(CFLAGS) $(LDFLAGS) -C $(TARGET).C -O $(TARGET).O

all: $(TARGET)

.PHONY: clean
clean:
	rm -rf *.o $(TARGET)
