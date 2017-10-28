CC ?= cc
#CFLAGS = -std=c99 -pedantic -Wall -Wextra -O3 -D_XOPEN_SOURCE=500
CFLAGS = -std=c99 -pedantic -Wall -Wextra -O3
CFLAGS += -DSSE2 -msse2 -I/usr/src/gnu/llvm/tools/clang/lib/Headers -Wno-pedantic
CFLAGS += -DAVX -mavx2 -I/usr/src/gnu/llvm/tools/clang/lib/Headers -Wno-pedantic

#BINS = shmff dummy invert grey crop kernel gauss scale
BINS = shmff dummy invert grey crop kernel gauss

.PHONY: all install clean test
all: ff2shm shm2ff dummy
clean:
	rm -f $(BINS) *.o *.core

ff2shm: ff2shm.c shmff.h
	$(CC) $(CFLAGS) -o $@ ff2shm.c

shm2ff: shm2ff.c shmff.h libshmff.o
	$(CC) $(CFLAGS) -o $@ shm2ff.c libshmff.o

install: ff2shm shm2ff dummy
	cp ff2shm shm2ff dummy $$HOME/bin

dummy: dummy.c shmff.h libshmff.o
	$(CC) $(CFLAGS) -o $@ dummy.c libshmff.o

# OLD STUFF #

test: shmff invert grey dummy in.ff
	./test.sh

shmff: shmff.c ff.h
	$(CC) $(CFLAGS) -o $@ shmff.c

invert: invert.c ff.h libshmff.o
	$(CC) $(CFLAGS) -o $@ invert.c libshmff.o

grey: grey.c ff.h libshmff.o
	$(CC) $(CFLAGS) -o $@ grey.c libshmff.o

crop: crop.c ff.h libshmff.o
	$(CC) $(CFLAGS) -o $@ crop.c libshmff.o

kernel: kernel.c ff.h libshmff.o
	$(CC) $(CFLAGS) -o $@ kernel.c libshmff.o

gauss: gauss.c ff.h libshmff.o
	$(CC) $(CFLAGS) -o $@ gauss.c libshmff.o

scale: scale.c ff.h libshmff.o
	$(CC) $(CFLAGS) -o $@ scale.c libshmff.o

libshmff.o: libshmff.c shmff.h
	$(CC) $(CFLAGS) -c -o $@ libshmff.c

# assembeler code

gauss.S: gauss.c ff.h
	$(CC) $(CFLAGS) -S -c -o $@ gauss.c

invert.S: invert.c ff.h
	$(CC) $(CFLAGS) -S -c -o $@ invert.c

