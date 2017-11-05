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
#include <string.h>
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

	double kern[3][3] = {
/* soble */
		{ 1.0, 0.0, -1.0},
		{ 2.0, 0.0, -2.0},
		{ 1.0, 0.0, -1.0},

/* gauss */
/*
		{ 1.0/16.0, 1.0/8.0, 1.0/16.0},
		{ 1.0/ 8.0, 1.0/4.0, 1.0/ 8.0},
		{ 1.0/16.0, 1.0/8.0, 1.0/16.0},
*/
	};

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

	setshmff(&hdr_r, &ff_r, 0);
	setshmff(&hdr_w, &ff_w, 1);
	memmove(hdr_w, hdr_r, sizeof *hdr_r);

	size_t px_n = hdr_r->width * hdr_r->height;
	size_t off = 0;

	int child = fork_jobs(jobs, &off, &px_n);

	for (size_t p = off; p < px_n; p++) {
		double r = 0.0;
		double g = 0.0;
		double b = 0.0;

		/* first row **************************************************/
		if (p >= hdr_r->width) {
			if (p % hdr_r->width >= 1) {
				r += kern[0][0] * ff_r[p - hdr_r->width -1].red;
				g += kern[0][0] * ff_r[p - hdr_r->width -1].green;
				b += kern[0][0] * ff_r[p - hdr_r->width -1].blue;
			}

			r += kern[0][1] * ff_r[p - hdr_r->width].red;
			g += kern[0][1] * ff_r[p - hdr_r->width].green;
			b += kern[0][1] * ff_r[p - hdr_r->width].blue;

			if (p % hdr_r->width < hdr_r->width - 1) {
				r += kern[0][2] * ff_r[p - hdr_r->width +1].red;
				g += kern[0][2] * ff_r[p - hdr_r->width +1].green;
				b += kern[0][2] * ff_r[p - hdr_r->width +1].blue;
			}
		}

		/* second row *************************************************/
		if (p % hdr_r->width >= 1) {
			r += kern[1][0] * ff_r[p - 1].red;
			g += kern[1][0] * ff_r[p - 1].red;
			b += kern[1][0] * ff_r[p - 1].red;
		}

		r += kern[1][1] * ff_r[p].red;
		g += kern[1][1] * ff_r[p].green;
		b += kern[1][1] * ff_r[p].blue;

		if (p % hdr_r->width < hdr_r->width - 1) {
			r += kern[1][2] * ff_r[p + 1].red;
			g += kern[1][2] * ff_r[p + 1].green;
			b += kern[1][2] * ff_r[p + 1].blue;
		}

		/* third row **************************************************/
		if (p / hdr_r->width < hdr_r->height - 1) {
			if (p % hdr_r->width >= 1) {
				r += kern[2][0] * ff_r[p + hdr_r->width -1].red;
				g += kern[2][0] * ff_r[p + hdr_r->width -1].green;
				b += kern[2][0] * ff_r[p + hdr_r->width -1].blue;
			}

			r += kern[2][1] * ff_r[p + hdr_r->width].red;
			g += kern[2][1] * ff_r[p + hdr_r->width].green;
			b += kern[2][1] * ff_r[p + hdr_r->width].blue;

			if (p % hdr_r->width < hdr_r->width - 1) {
				r += kern[2][2] * ff_r[p + hdr_r->width +1].red;
				g += kern[2][2] * ff_r[p + hdr_r->width +1].green;
				b += kern[2][2] * ff_r[p + hdr_r->width +1].blue;
			}
		}

		ff_w[p].red   = r;
		ff_w[p].green = g;
		ff_w[p].blue  = b;
		ff_w[p].alpha = ff_r[p].alpha;
	}

	return catch_jobs(jobs, child);
}
