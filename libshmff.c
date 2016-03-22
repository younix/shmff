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

void
setshmff(struct hdr **hdr_r, struct hdr **hdr_w, struct px **ff_r, struct px **ff_w)
{
	struct shmff shmff_r;
	struct shmff shmff_w;
	int shmid;

	if (fread(&shmff_r, sizeof shmff_r, 1, stdin) < 1)
		err(EXIT_FAILURE, "unable to read ff shm header");
	if (fread(&shmff_w, sizeof shmff_w, 1, stdin) < 1)
		err(EXIT_FAILURE, "unable to read ff shm header");

	/* Locate and attach ff shm segment for reading */
	if ((shmid = shmget(shmff_r.key, shmff_r.size, 0)) == -1)
		err(EXIT_FAILURE, "shmget");
	if ((*hdr_r = shmat(shmid, NULL, 0)) == (void *) -1)
		err(EXIT_FAILURE, "shmat");

	/* Locate and attach ff shm segment for reading */
	if ((shmid = shmget(shmff_w.key, shmff_w.size, 0)) == -1)
		err(EXIT_FAILURE, "shmget");
	if ((*hdr_w = shmat(shmid, NULL, 0)) == (void *) -1)
		err(EXIT_FAILURE, "shmat");

	/* prepare hdr on writing page */
	memmove(*hdr_w, *hdr_r, sizeof **hdr_r);

	*ff_r = (struct px *)(*hdr_r + 1);
	*ff_w = (struct px *)(*hdr_w + 1);
}

int
fork_jobs(int jobs, size_t *off, size_t *px_n)
{
	size_t job_len = *px_n / jobs;

	for (; jobs > 1; jobs--) {
		switch (fork()) {
		case -1:
			err(EXIT_FAILURE, "fork");
		case 0:
			*off += job_len;
			break;
		default:
			*px_n = *off + job_len;
			return 0;
		}
	}

	return 1;
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
