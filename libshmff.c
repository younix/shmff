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
setshmff(struct hdr **hdr, struct px **ff)
{
	struct shmff shmff;
	int shmid;

	if (fread(&shmff, sizeof shmff, 1, stdin) < 1)
		err(EXIT_FAILURE, "unable to read ff shm header");

	if (memcmp(shmff.magic, "sharedff", sizeof(shmff.magic)) == 0)
		errx(EXIT_FAILURE, "got incorrect header");

	if ((shmid = shmget(shmff.key, shmff.size, 0)) == -1)
		err(EXIT_FAILURE, "shmget");

	if ((*hdr = shmat(shmid, NULL, 0)) == (void *) -1)
		err(EXIT_FAILURE, "shmat");

	*ff = (struct px *)(*hdr + 1);
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
