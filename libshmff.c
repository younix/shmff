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

#include "shmff.h"

int
shmff_read(struct shmff *shmff, struct hdr **hdr, struct px **ff)
{
	int shmid;

	if (fread(shmff, sizeof *shmff, 1, stdin) < 1)
		return -1;

	if (memcmp(shmff->magic, "sharedff", sizeof(shmff->magic)) != 0)
		return -1;	/* incorrect header */

	if ((shmid = shmget(shmff->key, shmff->size, 0)) == -1) {
fprintf(stderr, "%s:%d\n", __func__, __LINE__);
perror("shmget");
		return -1;
	}

	if ((*hdr = shmat(shmid, NULL, 0)) == (void *) -1) {
fprintf(stderr, "%s:%d\n", __func__, __LINE__);
		return -1;
}

	*ff = (struct px *)(*hdr + 1);

	return 0;
}

int
shmff_write(struct shmff *shmff)
{
	if (fwrite(shmff, sizeof *shmff, 1, stdout) < 1)
		return -1;

	return 0;
}

int
shmff_free(struct shmff *shmff, struct hdr *hdr)
{
	int shmid;

	if ((shmid = shmget(shmff->key, shmff->size, 0)) == -1)
		return -1;

	/* delete */
	if (shmctl(shmid, IPC_RMID, NULL) == -1)
		return -1;

	/* unmap */
	if (shmdt(hdr) == -1)
		return -1;

	return 0;
}

int
fork_jobs(int jobs, size_t *off, size_t *px_n)
{
	size_t job_len = *px_n / jobs;

	for (; jobs > 1; jobs--) {
		switch (fork()) {
		case -1:
			return -1;
		case 0:	/* child */
			*off += job_len;
			return 1;
		default: /* parent */
			*px_n = *off + job_len;
			break;
		}
	}

	return 0;
}

int
catch_jobs(int jobs, int child)
{
	int status;

	if (child == 0) {
		for (;jobs > 1; jobs--) {
			wait(&status);
			if (status != EXIT_SUCCESS)
				return EXIT_FAILURE;
		}
	}

	return EXIT_SUCCESS;
}

int
file2shm(const char *file, struct shmff *shmff, struct hdr **hdr, struct px **ff)
{
	int shmid = -1;
	FILE *fh = stdin;
	struct hdr fhdr;	/* file header */

	if (file != NULL && (fh = fopen(file, "r")) == NULL)
		err(EXIT_FAILURE, "fopen");

	/* read header */
	if (fread(&fhdr, sizeof fhdr, 1, fh) < 1)
		err(EXIT_FAILURE, "fread");

	/* prepare shmff */
	memcpy(&shmff->magic, "sharedff", sizeof shmff->magic);
	shmff->key = ftok(file, 0);	/* gen. name of shm seg. by file path */
	shmff->size = sizeof(**hdr) +
	    sizeof(**ff) * ntohl(fhdr.width) * ntohl(fhdr.height);

	/* TODO: check shm/mem limits */
 again:
	/* Create and attach the shm segment for reading. */
	if ((shmid = shmget(shmff->key, shmff->size, IPC_CREAT | 0600)) == -1) {
		if (errno == ENOMEM) {
			fprintf(stderr, "ENOMEM sleep(1)\n");
			sleep(1);
			errno = 0;
			goto again;
		} else {
			err(EXIT_FAILURE, "shmget: size: %zu", shmff->size);
		}
	}

	if ((*hdr = shmat(shmid, NULL, 0)) == (void *) -1)
		err(EXIT_FAILURE, "shmat");

	/* prepare farbfeld header */
	memcpy(&(*hdr)->magic, &fhdr.magic, sizeof fhdr.magic);
	(*hdr)->width = ntohl(fhdr.width);
	(*hdr)->height = ntohl(fhdr.height);

	*ff = (struct px *)(*hdr + 1);

	size_t px_n = (*hdr)->width * (*hdr)->height;
	for (size_t p = 0; p < px_n; p++) {
		if (fread(&(*ff)[p], sizeof (*ff)[p], 1, fh) < 1)
			err(EXIT_FAILURE, "unexpected end-of-file");

		/* convert net endian to host endian */
		(*ff)[p].red   = ntohs((*ff)[p].red);
		(*ff)[p].green = ntohs((*ff)[p].green);
		(*ff)[p].blue  = ntohs((*ff)[p].blue);
		(*ff)[p].alpha = ntohs((*ff)[p].alpha);
	}

	if (file != NULL && fclose(fh) == EOF)
		err(EXIT_FAILURE, "fclose");

	return shmid;
}

void
shm2file(const char *file, struct hdr *hdr, struct px *ff)
{
	FILE *fh = stdout;
	size_t px_n = hdr->width * hdr->height;

	hdr->width = htonl(hdr->width);
	hdr->height = htonl(hdr->height);

	if (file != NULL && (fh = fopen(file, "w")) == NULL)
			err(EXIT_FAILURE, "fopen");

	if (fwrite(hdr, sizeof *hdr, 1, fh) < 1)	/* save header */
		err(EXIT_FAILURE, "fwrite");

	for (size_t p = 0; p < px_n; p++) {
		/* convert host endian to net endian */
		ff[p].red   = htons(ff[p].red);
		ff[p].green = htons(ff[p].green);
		ff[p].blue  = htons(ff[p].blue);
		ff[p].alpha = htons(ff[p].alpha);

		if (fwrite(&ff[p], sizeof *ff, 1, fh) < 1)
			err(EXIT_FAILURE, "unexpected end-of-file");
	}

	if (file != NULL && fclose(fh) == EOF)
		err(EXIT_FAILURE, "fclose");
}
