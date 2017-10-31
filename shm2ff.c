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

#if __x86_64__ && (SSE2 || AVX)
#include <immintrin.h>
#endif

#include "shmff.h"

static int verbose = 0;
static struct timespec start;

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
	char *file = NULL;
	struct shmff shmff;
	struct hdr *hdr = NULL;
	struct px *ff = NULL;

	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);

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

	if (isatty(fileno(stdin)))
		errx(EXIT_FAILURE, "input is a terminal");

	if (argc < 1)
		usage();

	file = argv[0];

	if (shmff_load(&shmff, &hdr, &ff) == -1)
		err(EXIT_FAILURE, "shmff_load");

	FILE *fh = stdout;
	size_t px_n = hdr->width * hdr->height;
	size_t p = 0;

	if (file != NULL) {
		if ((fh = fopen(file, "w")) == NULL)
			err(EXIT_FAILURE, "fopen");
	}

	if (isatty(fileno(fh)))
		errx(EXIT_FAILURE, "output is a terminal");

	hdr->width = htonl(hdr->width);
	hdr->height = htonl(hdr->height);

	/* save header */
	if (fwrite(hdr, sizeof *hdr, 1, fh) < 1)
		err(EXIT_FAILURE, "fread");

	/* skip 16 byte (2 pixel) for alignment */
	for (; p < 2 && p < px_n; p++) {
		ff[p].red   = htons(ff[p].red);
		ff[p].green = htons(ff[p].green);
		ff[p].blue  = htons(ff[p].blue);
		ff[p].alpha = htons(ff[p].alpha);
	}

#if __x86_64__ && AVX
	/* TODO: check cpuid */
	debug(1, start, "endian conversion with AVX2 instructions");
	for (; (p + 4) < px_n; p += 4) {
		__m256i a;
		__m256i l;      /* left */
		__m256i r;      /* right */

		a = _mm256_load_si256((__m256i *)&ff[p]);

		l = _mm256_slli_epi16(a, 8);
		r = _mm256_srli_epi16(a, 8);
		a = _mm256_or_si256(l, r);

		_mm256_store_si256((__m256i *)&ff[p], a);
	}
#endif
#if __x86_64__ && SSE2
	/* TODO: check cpuid */
	debug(1, start, "endian conversion with SSE2 instructions");
	for (; (p + 2) < px_n; p += 2) {
		__m128i a;
		__m128i l;	/* left */
		__m128i r;	/* right */

		a = _mm_load_si128((__m128i *)&ff[p]);

		l = _mm_slli_epi16(a, 8);
		r = _mm_srli_epi16(a, 8);
		a = _mm_or_si128(l, r);

		_mm_store_si128((__m128i *)&ff[p], a);
	}
#endif

	for (; p < px_n; p++) {
		/* convert host endian to net endian */
		ff[p].red   = htons(ff[p].red);
		ff[p].green = htons(ff[p].green);
		ff[p].blue  = htons(ff[p].blue);
		ff[p].alpha = htons(ff[p].alpha);
	}

	debug(1, start, "writing image data to file");

	if (fwrite(&ff[0], sizeof *ff, px_n, fh) < px_n)
		err(EXIT_FAILURE, "unexpected end-of-file");

	if (fclose(fh) == EOF)
		err(EXIT_FAILURE, "fclose");

	shmff_free(&shmff, hdr);

	debug(1, start, "file");

	return EXIT_SUCCESS;
}
