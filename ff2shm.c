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

void
usage(void)
{
	fputs("ff2shm [-vh] [file]\n", stderr);
	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	int ch;
	int verbose = 0;
	struct shmff shmff;
	struct hdr *hdr;
	struct px *ff;

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

	for (; argc > 0; argc--, argv++) {
		if (verbose >= 1)
			fprintf(stderr, "import: %s\n", argv[0]);
		if (file2shm(argv[0], &shmff, &hdr, &ff) == -1)
			err(EXIT_FAILURE, "file2shm: %s", argv[0]);
		if (shmff_write(&shmff) == -1)
			err(EXIT_FAILURE, "file2shm: %s", argv[0]);
		if (shmdt(hdr) == -1)
			err(EXIT_FAILURE, "shmdt");
		if (fflush(stdout) != 0)
			err(EXIT_FAILURE, "fflush");
	}

	return EXIT_SUCCESS;
}
