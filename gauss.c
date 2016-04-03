#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <err.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ff.h"
#include "libshmff.h"

void
usage(void)
{
	fputs("invert [-j njobs]\n", stderr);
	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	struct px *ff_r, *ff_w;
	struct hdr *hdr_r = NULL, *hdr_w = NULL;
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

	setshmff(&hdr_r, &hdr_w, &ff_r, &ff_w);

	size_t px_n = hdr_r->width * hdr_r->height;
	size_t off = 0;

	int child = fork_jobs(jobs, &off, &px_n);

	for (size_t p = off; p < px_n; p++) {
		uint32_t r = 0;
		uint32_t g = 0;
		uint32_t b = 0;

		/* first row **************************************************/
		if (p >= hdr_r->width) {
			if (p % hdr_r->width >= 1) {
				r += ff_r[p - hdr_r->width -1].red;
				g += ff_r[p - hdr_r->width -1].green;
				b += ff_r[p - hdr_r->width -1].blue;
			}

			r += ff_r[p - hdr_r->width].red   << 1;
			g += ff_r[p - hdr_r->width].green << 1;
			b += ff_r[p - hdr_r->width].blue  << 1;

			if (p % hdr_r->width < hdr_r->width - 1) {
				r += ff_r[p - hdr_r->width +1].red;
				g += ff_r[p - hdr_r->width +1].green;
				b += ff_r[p - hdr_r->width +1].blue;
			}
		}

		/* second row *************************************************/
		if (p % hdr_r->width >= 1) {
			r += ff_r[p - 1].red << 1;
			g += ff_r[p - 1].red << 1;
			b += ff_r[p - 1].red << 1;
		}

		r += ff_r[p].red   << 2;
		g += ff_r[p].green << 2;
		b += ff_r[p].blue  << 2;

		if (p % hdr_r->width < hdr_r->width - 1) {
			r += ff_r[p + 1].red   << 1;
			g += ff_r[p + 1].green << 1;
			b += ff_r[p + 1].blue  << 1;
		}

		/* third row **************************************************/
		if (p / hdr_r->width < hdr_r->height - 1) {
			if (p % hdr_r->width >= 1) {
				r += ff_r[p + hdr_r->width -1].red;
				g += ff_r[p + hdr_r->width -1].green;
				b += ff_r[p + hdr_r->width -1].blue;
			}

			r += ff_r[p + hdr_r->width].red   << 1;
			g += ff_r[p + hdr_r->width].green << 1;
			b += ff_r[p + hdr_r->width].blue  << 1;

			if (p % hdr_r->width < hdr_r->width - 1) {
				r += ff_r[p + hdr_r->width +1].red;
				g += ff_r[p + hdr_r->width +1].green;
				b += ff_r[p + hdr_r->width +1].blue;
			}
		}

		ff_w[p].red   = r >> 4;
		ff_w[p].green = g >> 4;
		ff_w[p].blue  = b >> 4;
		ff_w[p].alpha = ff_r[p].alpha;
	}

	return catch_jobs(jobs, child);
}
