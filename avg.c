#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#include "shmff.h"

#define MIN(a, b)\
	((a) < (b) ? (a) : (b))

int
main(void)
{
	struct shmff shmff;
	struct hdr *hdr;
	struct px *ff;
	struct shmff avg_shmff;
	struct hdr *avg_hdr = NULL;
	struct px *avg_ff = NULL;
	size_t count = 0;

	struct map {
		uint64_t red;
		uint64_t green;
		uint64_t blue;
	} *map = NULL;

	for (;; count++) {
		/* read shmff structure from stdin */
		if (shmff_read(&shmff, &hdr, &ff) == -1) {
			if (feof(stdin))
				break;
			err(EXIT_FAILURE, "shmff_read");
		}

		if (map == NULL) {
			map = calloc(hdr->width * hdr->height, sizeof *map);
			if (map == NULL)
				err(EXIT_FAILURE, "calloc");
			memcpy(&avg_shmff, &shmff, sizeof shmff);
			avg_hdr = hdr;
			avg_ff = ff;
			for (size_t i = 0; i < hdr->width * hdr->height; i++) {
				map[i].red   = ff[i].red;
				map[i].green = ff[i].green;
				map[i].blue  = ff[i].blue;
			}
			continue;
		}

//		if (avg_hdr == NULL && avg_ff == NULL) {
//			memcpy(&avg_shmff, &shmff, sizeof shmff);
//			avg_hdr = hdr;
//			avg_ff = ff;
//			continue;
//		}

		if (hdr->width != avg_hdr->width ||
		    hdr->height != avg_hdr->height)
			goto err2;

		for (size_t i = 0; i < hdr->width * hdr->height; i++) {
			if (UINT64_MAX - map[i].red < ff[i].red ||
			    UINT64_MAX - map[i].green < ff[i].green ||
			    UINT64_MAX - map[i].blue < ff[i].blue)
				errx(EXIT_FAILURE, "overflow");
#if 1
			map[i].red   += ff[i].red;
			map[i].green += ff[i].green;
			map[i].blue  += ff[i].blue;
#else
			map[i].red   = MIN(map[i].red, ff[i].red);
			map[i].green = MIN(map[i].green, ff[i].green);
			map[i].blue  = MIN(map[i].blue, ff[i].blue);
#endif
		}

		if (shmff_free(&shmff, hdr) == -1)
			perror("shmff_free");
		fprintf(stderr, "counter: %zu\n", count);
	}

fprintf(stderr, "count: %zu\n", count);
	if (avg_hdr == NULL)
		return EXIT_FAILURE;

	for (size_t i = 0; i < avg_hdr->width * avg_hdr->height; i++) {
#if 1
		avg_ff[i].red   = map[i].red / count;
		avg_ff[i].green = map[i].green / count;
		avg_ff[i].blue  = map[i].blue / count;
#else
		avg_ff[i].red   = map[i].red;
		avg_ff[i].green = map[i].green;
		avg_ff[i].blue  = map[i].blue;
#endif
	}

	free(map);

	/* write shmff structure to stdout */
	if (shmff_write(&avg_shmff) == -1)
		goto err;

	return EXIT_SUCCESS;
 err2:
	if (shmff_free(&shmff, hdr) == -1)
		perror("shmff_free");
 err:
	if (shmff_free(&avg_shmff, avg_hdr) == -1)
		perror("shmff_free");
	return EXIT_FAILURE;
}
