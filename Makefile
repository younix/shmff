CC ?= cc
CFLAGS = -std=c99 -pedantic -Wall -Wextra

.PHONY: all clean
all: shmff invert
clean:
	rm -f shmff invert

shmff: shmff.c
	$(CC) $(CFLAGS) -o $@ shmff.c

invert: invert.c
	$(CC) $(CFLAGS) -o $@ invert.c
