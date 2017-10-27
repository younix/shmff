#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <err.h>
#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "shmff.h"

#define debug(level, ...) 			\
	if ((level) >= verbose)			\
		fprintf(stderr, __VA_ARGS__);

static int verbose = 0;

void
usage(void)
{
	fputs("ff2shm [-h] [file]\n", stderr);
	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	int shmid;
	int ch;
	key_t key;	/* read page */
	FILE *fh = stdin;
	char *file = NULL;

	struct hdr hdr;
	struct hdr *shm;
	struct px *ff = NULL;

	while ((ch = getopt(argc, argv, "vh")) != -1) {
		switch (ch) {
		case 'v':
			verbose++;
			break;
		case 'h':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (isatty(fileno(stdout)))
		errx(EXIT_FAILURE, "output on a terminal");
	errno = 0;

	if (argc < 1)
		usage();

	/* TODO: add use of stdin here */

	file = argv[0];

	if ((fh = fopen(file, "r")) == NULL)
		err(EXIT_FAILURE, "fopen: %s", file);

	/* read header */
	if (fread(&hdr, sizeof hdr, 1, fh) < 1)
		err(EXIT_FAILURE, "fread: %s", file);

	/* check magic string */
	if (memcmp(hdr.magic, "farbfeld", sizeof(hdr.magic)) != 0)
		errx(EXIT_FAILURE, "file: %s is not in ff format", file);

	hdr.width = ntohl(hdr.width);
	hdr.height = ntohl(hdr.height);

	size_t px_n = hdr.width * hdr.height;
	size_t ff_size = sizeof(hdr) + hdr.width * hdr.height * sizeof(*ff);

	/* XXX: check shmmax limit */

	/* generate name of shm segment by file path */
	key = ftok(file, 3);

	debug(1, "key: %lu\n", key);
	debug(1, "size: %zu\n", ff_size);

	/* Create and attach the shm segment for reading. */
	if ((shmid = shmget(key, ff_size, IPC_CREAT | 0600)) == -1)
		err(EXIT_FAILURE, "shmget");

	if ((shm = shmat(shmid, NULL, 0)) == (void *) -1)
		err(EXIT_FAILURE, "shmat");

	memcpy(shm, &hdr, sizeof hdr);

	ff = (struct px *)(shm + 1);

	/* read image into memory */
	for (size_t p = 0; p < px_n; p++) {
		if (fread(&ff[p], sizeof *ff, 1, fh) < 1)
			err(EXIT_FAILURE, "unexpected end-of-file");

		/* convert net endian to host endian */
		ff[p].red   = ntohs(ff[p].red);
		ff[p].green = ntohs(ff[p].green);
		ff[p].blue  = ntohs(ff[p].blue);
		ff[p].alpha = ntohs(ff[p].alpha);
	}

	if (fclose(fh) == EOF)
		err(EXIT_FAILURE, "fclose");

	struct shmff shmff;

	shmff.size = ff_size;
	shmff.key = key;

	if (fwrite(&shmff, sizeof shmff, 1, stdout) < 1)
		goto err;

	return EXIT_SUCCESS;

 err:
	/* unmap */
	if (shmdt(shm) == -1)
		err(EXIT_FAILURE, "shmdt");

	/* delete */
	if (shmctl(shmid, IPC_RMID, NULL) == -1)
		err(EXIT_FAILURE, "shmctl");

	err(EXIT_FAILURE, "error");
}
