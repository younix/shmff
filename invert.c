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

#ifdef SSE
#include <emmintrin.h>
#endif

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

	setshmff(&hdr_r, &ff_r);
	setshmff(&hdr_w, &ff_w);
	memmove(hdr_w, hdr_r, sizeof *hdr_r);

	size_t px_n = hdr_r->width * hdr_r->height;
	size_t off = 0;

	int child = fork_jobs(jobs, &off, &px_n);

#ifdef SSE
	__m128i op = _mm_set_epi16(0, UINT16_MAX, UINT16_MAX, UINT16_MAX,
	                           0, UINT16_MAX, UINT16_MAX, UINT16_MAX);

	__m128i al = _mm_set_epi16(UINT16_MAX, 0, 0, 0, UINT16_MAX, 0, 0, 0);

	size_t p_len = px_n - off;
	size_t i = (sizeof(*ff_r) * (p_len)) / sizeof(op);

	for (; i > 0; i--) {
		__m128i P = _mm_loadu_si128((__m128i *)&ff_r[off]);
		__m128i C = _mm_subs_epu16(op, P);
		C = _mm_adds_epu16(C, al);
		_mm_storeu_si128((__m128i *)&ff_w[off], C);
		off += 2;
	}
#endif

	for (size_t p = off; p < px_n; p++) {
		/* invert colors */
		ff_w[p].red   = UINT16_MAX - ff_r[p].red;
		ff_w[p].green = UINT16_MAX - ff_r[p].green;
		ff_w[p].blue  = UINT16_MAX - ff_r[p].blue;
		ff_w[p].alpha = ff_r[p].alpha;
	}

	return catch_jobs(jobs, child);
}
