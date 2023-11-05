CC = gcc
CFLAGS = -Wall -g

SOURCES = simpleshell.c.c fib.c helloworld.c
EXECUTABLES = $(SOURCES:.c=)

all: $(EXECUTABLES)

%: %.c
	$(CC) $(CFLAGS) -o $@ $<

run: all
	./simpleshell.c.c

clean:
	rm -f $(EXECUTABLES)

.PHONY: all run clean