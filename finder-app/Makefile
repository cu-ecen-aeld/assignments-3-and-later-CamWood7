all: writer

clean:
	rm -rf *.o
	rm -f writer

writer: writer.o
	$(CC) $(LDFLAGS) -o writer writer.o

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

.PHONY: clean all
