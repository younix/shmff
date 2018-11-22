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
	fputs("shm2ff [-h] [file]\n", stderr);
	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	int verbose = 0;
	int ch;
	char *file = NULL;
	struct shmff shmff;
	struct hdr *hdr = NULL;
	struct px *ff = NULL;

	while ((ch = getopt(argc, argv, "h")) != -1) {
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

	if (isatty(fileno(stdin)))
		errx(EXIT_FAILURE, "input is a terminal");

	if (argc < 1)
		usage();

	file = argv[0];

	if (shmff_read(&shmff, &hdr, &ff) == -1)
		err(EXIT_FAILURE, "shmff_read");

	shm2file(file, hdr, ff);
	shmff_free(&shmff, hdr);

	return EXIT_SUCCESS;
}
