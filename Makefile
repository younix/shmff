CC ?= cc
CFLAGS = -std=c99 -pedantic -Wall -Wextra -g

BINS = shmff dummy invert grey

.PHONY: all clean test
all: $(BINS)
clean:
	rm -f $(BINS) *.o *.core

test: shmff invert grey dummy in.ff
	./test.sh

shmff: shmff.c ff.h
	$(CC) $(CFLAGS) -o $@ shmff.c

invert: invert.c ff.h libshmff.o
	$(CC) $(CFLAGS) -o $@ invert.c libshmff.o

grey: grey.c ff.h libshmff.o
	$(CC) $(CFLAGS) -o $@ grey.c libshmff.o

dummy: dummy.c ff.h libshmff.o
	$(CC) $(CFLAGS) -o $@ dummy.c libshmff.o

libshmff.o: libshmff.c
	$(CC) $(CFLAGS) -c -o $@ libshmff.c
