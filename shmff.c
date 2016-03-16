#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ff.h"

#define KEY_ID_READ  0	/* read shm id */
#define KEY_ID_WRITE 1	/* write shm id */

void
shm2file(const char *file, struct hdr *hdr, struct px *ff)
{
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
		/* convert net endian to host endian */
		ff[p].red   = htons(ff[p].red);
		ff[p].green = htons(ff[p].green);
		ff[p].blue  = htons(ff[p].blue);
		ff[p].alpha = htons(ff[p].alpha);

		if (fwrite(&ff[p], sizeof *ff, 1, fh) < 1)
			err(EXIT_FAILURE, "unexpected end-of-file");
	}

	if (fclose(fh) == EOF)
		err(EXIT_FAILURE, "fclose");
}

void
usage(void)
{
	fputs("shmff <in file> <out file>\n", stderr);
	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	int shmid;
	key_t key_r;	/* read page */
	key_t key_w;	/* write page */
	FILE *fh = stdin;
	FILE *ph;
	char *file = NULL;

	struct hdr hdr;
	struct px *ff_r = NULL;
	struct px *ff_w = NULL;

	if (argc >= 2)
		file = argv[1];


	if (file != NULL)
		if ((fh = fopen(file, "r")) == NULL)
			err(EXIT_FAILURE, "fopen");

	/* read header */
	if (fread(&hdr, sizeof hdr, 1, fh) < 1)
		err(EXIT_FAILURE, "fread");

	/* check magic string */
	if (memcmp(hdr.magic, "farbfeld", sizeof(hdr.magic)) != 0)
		err(EXIT_FAILURE, "file is not in ff format");

	hdr.width = ntohl(hdr.width);
	hdr.height = ntohl(hdr.height);

	size_t px_n = hdr.width * hdr.height;
	size_t ff_size = hdr.width * hdr.height * sizeof(struct px);
	fprintf(stderr, "ff mem size: %zu\n", ff_size);

	/* generate name of shm segment by file path */
	key_r = ftok(file, KEY_ID_READ);
	key_w = ftok(file, KEY_ID_WRITE);

	/* Create and attach the shm segment for reading.  */
	if ((shmid = shmget(key_r, ff_size, IPC_CREAT | 0666)) < 0)
		err(EXIT_FAILURE, "shmget");
	if ((ff_r = shmat(shmid, NULL, 0)) == (void *) -1)
		err(EXIT_FAILURE, "shmat");

	/* Create and attach the shm segment for writing.  */
	if ((shmid = shmget(key_w, ff_size, IPC_CREAT | 0666)) < 0)
		err(EXIT_FAILURE, "shmget");
	if ((ff_w = shmat(shmid, NULL, 0)) == (void *) -1)
		err(EXIT_FAILURE, "shmat");

	/* read image into memory */
	for (size_t p = 0; p < px_n; p++) {
		if (fread(&ff_r[p], sizeof *ff_r, 1, fh) < 1)
			err(EXIT_FAILURE, "unexpected end-of-file");

		/* convert net endian to host endian */
		ff_r[p].red   = ntohs(ff_r[p].red);
		ff_r[p].green = ntohs(ff_r[p].green);
		ff_r[p].blue  = ntohs(ff_r[p].blue);
		ff_r[p].alpha = ntohs(ff_r[p].alpha);
	}

	if (fclose(fh) == EOF)
		err(EXIT_FAILURE, "fclose");

	/*
	 * start converting program
	 */
	if ((ph = popen("./invert", "w")) == NULL)
		err(EXIT_FAILURE, "popen");

	struct shm_ff shmff_r;
	struct shm_ff shmff_w;

	shmff_r.width  = shmff_w.width  = hdr.width;
	shmff_r.height = shmff_w.height = hdr.height;
	shmff_r.size   = shmff_w.size   = ff_size;
	shmff_r.key = key_r;
	shmff_w.key = key_w;

	fwrite(&shmff_r, sizeof shmff_r, 1, ph);
	fwrite(&shmff_w, sizeof shmff_w, 1, ph);

	if (pclose(ph) != EXIT_SUCCESS)
		err(EXIT_FAILURE, "pclose");

	/*
	 * save image
	 */
	if (argc >= 3)
		file = argv[2];
	else
		file = NULL;

	shm2file(file, &hdr, ff_w);

	exit(0);
}
