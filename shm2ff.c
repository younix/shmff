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
	fputs("shm2ff [file]\n", stderr);
	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	int ch;
	int verbose = 0;
	char *file = NULL;
	struct shmff shmff;
	struct hdr *hdr = NULL;
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

	/* TODO: check that stdin is not a terminal */

	if (argc < 1)
		usage();

	/* TODO: add use of stdout here */

	file = argv[0];

	shmff_load(&shmff, &hdr, &ff);

	FILE *fh = stdout;
	size_t px_n = hdr->width * hdr->height;

	if (file != NULL) {
		if ((fh = fopen(file, "w")) == NULL)
			err(EXIT_FAILURE, "fopen");
	}

	hdr->width = htonl(hdr->width);
	hdr->height = htonl(hdr->height);

	/* save header */
	if (fwrite(hdr, sizeof *hdr, 1, fh) < 1)
		err(EXIT_FAILURE, "fread");

	for (size_t p = 0; p < px_n; p++) {
		/* convert host endian to net endian */
		ff[p].red   = htons(ff[p].red);
		ff[p].green = htons(ff[p].green);
		ff[p].blue  = htons(ff[p].blue);
		ff[p].alpha = htons(ff[p].alpha);

		if (fwrite(&ff[p], sizeof *ff, 1, fh) < 1)
			err(EXIT_FAILURE, "unexpected end-of-file");
	}

	if (fclose(fh) == EOF)
		err(EXIT_FAILURE, "fclose");

	shmff_free(&shmff, hdr);

	return EXIT_SUCCESS;
}
