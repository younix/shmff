CC ?= cc
CFLAGS = -static -std=c99 -pedantic -Wall -Wextra

BINS = shmff dummy invert grey

.PHONY: all clean test
all: $(BINS)
clean:
	rm -f $(BINS)

test: shmff invert grey in.ff
	shmff < in.ff > out.ff

shmff: shmff.c
	$(CC) $(CFLAGS) -o $@ shmff.c

invert: invert.c
	$(CC) $(CFLAGS) -o $@ invert.c

grey: grey.c
	$(CC) $(CFLAGS) -o $@ grey.c
