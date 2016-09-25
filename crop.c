#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>

#include <err.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ff.h"
#include "libshmff.h"

void
usage(void)
{
	fputs("grey [-j jobs] x y width height\n", stderr);
	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	struct hdr *hdr_r = NULL;
	struct hdr *hdr_w = NULL;
	struct px *ff_r = NULL;
	struct px *ff_w = NULL;
	unsigned int jobs = 1;
	int ch;

	while ((ch = getopt(argc, argv, "j:h")) != -1) {
		switch (ch) {
		case 'j':
			errno = 0;
			if ((jobs = strtoul(optarg, NULL, 0)) == 0) {
				if (errno != 0)
					err(EXIT_FAILURE, "strtoul");
				errx(EXIT_FAILURE, "invalid jobs");
			}
			break;
		case 'h':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc < 4)
		usage();

	size_t X = strtoul(argv[0], NULL, 0);
	size_t Y = strtoul(argv[1], NULL, 0);
	size_t W = strtoul(argv[2], NULL, 0);
	size_t H = strtoul(argv[3], NULL, 0);

	setshmff(&hdr_r, &ff_r);
	setshmff(&hdr_w, &ff_w);
	memmove(hdr_w, hdr_r, sizeof *hdr_r);

	hdr_w->width = W;
	hdr_w->height = H;

	for (size_t line = 0; line < H; line++) {
		size_t dst = line * W;
		size_t src = X + (Y + line) * hdr_r->width;

		memmove(&ff_w[dst], &ff_r[src], sizeof(*ff_w) * W);
	}

	return EXIT_SUCCESS;
}
