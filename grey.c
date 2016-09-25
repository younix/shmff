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
	fputs("grey [-j jobs]\n", stderr);
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

	setshmff(&hdr_r, &ff_r);
	setshmff(&hdr_w, &ff_w);
	memmove(hdr_w, hdr_r, sizeof *hdr_r);

	size_t px_n = hdr_r->width * hdr_r->height;
	size_t off = 0;
	int child = fork_jobs(jobs, &off, &px_n);

	for (size_t p = off; p < px_n; p++) {
		/* convert pixel to grey */
		ff_w[p].red = ff_w[p].green = ff_w[p].blue =
		   (ff_r[p].red   * 30 +
		    ff_r[p].green * 59 +
		    ff_r[p].blue  * 11) / 100;

		ff_w[p].alpha = ff_r[p].alpha;
	}

	return catch_jobs(jobs, child);
}
