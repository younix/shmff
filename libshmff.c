#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "shmff.h"

int
shmff_load(struct shmff *shmff, struct hdr **hdr, struct px **ff)
{
	int shmid;

	if (fread(shmff, sizeof *shmff, 1, stdin) < 1)
		return -1;

	if (memcmp(shmff->magic, "sharedff", sizeof(shmff->magic)) == 0)
		return -1;	/* incorrect header */

	if ((shmid = shmget(shmff->key, shmff->size, 0)) == -1)
		return -1;

	if ((*hdr = shmat(shmid, NULL, 0)) == (void *) -1)
		return -1;

	*ff = (struct px *)(*hdr + 1);

	return 0;
}

int
shmff_free(struct shmff *shmff, struct hdr *hdr)
{
	int shmid;

	/* unmap */
	if (shmdt(hdr) == -1)
		return -1;

	if ((shmid = shmget(shmff->key, shmff->size, 0)) == -1)
		return -1;

	/* delete */
	if (shmctl(shmid, IPC_RMID, NULL) == -1)
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
