CC = gcc
CFLAGS = -Wall -Werror -g
LDFLAGS += -Wl,--hash-style=gnu -pthread -lrt

all: aesdsocket

aesdsocket: aesdsocket.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -rf aesdsocket *.o
