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
//#include <immintrin.h>
#endif

#include "shmff.h"

static int verbose = 0;
static struct timespec start;

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
	key = ftok(file, 0);

	debug(1, start, "shm key: %lu", key);
	debug(1, start, "shm size: %zu", ff_size);

	/* Create and attach the shm segment for reading. */
	if ((shmid = shmget(key, ff_size, IPC_CREAT | 0600)) == -1)
		err(EXIT_FAILURE, "shmget");

	if ((shm = shmat(shmid, NULL, 0)) == (void *) -1)
		err(EXIT_FAILURE, "shmat");

	memcpy(shm, &hdr, sizeof hdr);

	ff = (struct px *)(shm + 1);

	/*
	 * read image data into memory and convert net endian to host endian
	 */
	size_t p = 0;

	debug(1, start, "read file content into memory");
	if (fread(&ff[0], sizeof *ff, px_n, fh) < px_n)
		err(EXIT_FAILURE, "unexpected end-of-file");
	debug(1, start,  "endian conversion");

	/* skip 16 byte (2 pixel) for alignment */
	for (; p < 2 && p < px_n; p++) {
		ff[p].red   = ntohs(ff[p].red);
		ff[p].green = ntohs(ff[p].green);
		ff[p].blue  = ntohs(ff[p].blue);
		ff[p].alpha = ntohs(ff[p].alpha);
//		ff[p].red, ff[p].green, ff[p].blue);
	}

#if __x86_64__ && AVX
	/* TODO: check cpuid */
	debug(1, start, "endian conversion with AVX2 instructions");
	for (; (p + 4) < px_n; p += 4) {
		__m256i a;
		__m256i l;	/* left */
		__m256i r;	/* right */

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
		a = _mm_or_si128 (l, r);

		_mm_store_si128((__m128i *)&ff[p], a);
	}
#endif

	for (; p < px_n; p++) {
		ff[p].red   = ntohs(ff[p].red);
		ff[p].green = ntohs(ff[p].green);
		ff[p].blue  = ntohs(ff[p].blue);
		ff[p].alpha = ntohs(ff[p].alpha);
	}

	if (fclose(fh) == EOF)
		err(EXIT_FAILURE, "fclose");

	struct shmff shmff;
	memcpy(&shmff.magic, "sharedff", sizeof shmff.magic);
	shmff.size = ff_size;
	shmff.key = key;

	if (fwrite(&shmff, sizeof shmff, 1, stdout) < 1)
		goto err;

	debug(1, start, "exit");
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
