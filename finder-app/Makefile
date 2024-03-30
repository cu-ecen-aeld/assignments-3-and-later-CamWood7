CC = gcc
CROSS_COMPILE ?=
CFLAGS = -Wall

SRC = writer.c
OBJ = $(SRC:.c=.o)
OUT = writer

ifdef CROSS_COMPILE
	CC = $(CROSS_COMPILE)gcc
endif

$(OUT): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(OUT)

.PHONY: clean
clean:
	rm -f $(OUT) $(OBJ)
