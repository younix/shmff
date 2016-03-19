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
#include <unistd.h>

#include "ff.h"

void
usage(void)
{
	fputs("invert [-j njobs]\n", stderr);
	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	struct shm_ff shmff_r;
	struct shm_ff shmff_w;
	struct px *ff_r;
	struct px *ff_w;
	int shmid;
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

	if (fread(&shmff_r, sizeof shmff_r, 1, stdin) < 1)
		err(EXIT_FAILURE, "unable to read ff shm header");

	if (fread(&shmff_w, sizeof shmff_w, 1, stdin) < 1)
		err(EXIT_FAILURE, "unable to read ff shm header");

	/* Locate and attach ff shm segment for reading */
	if ((shmid = shmget(shmff_r.key, shmff_r.size, 0666)) == -1)
		err(EXIT_FAILURE, "shmget");
	if ((ff_r = shmat(shmid, NULL, 0)) == (void *) -1)
		err(EXIT_FAILURE, "shmat");

	/* Locate and attach ff shm segment for reading */
	if ((shmid = shmget(shmff_w.key, shmff_w.size, 0666)) == -1)
		err(EXIT_FAILURE, "shmget");
	if ((ff_w = shmat(shmid, NULL, 0)) == (void *) -1)
		err(EXIT_FAILURE, "shmat");

	size_t px_n = shmff_r.width * shmff_r.height;
	size_t job_len = px_n / jobs;
	size_t off = 0;
	bool child = false;

	/* fork sub jobs */
	for (; jobs > 1; jobs--) {
		switch (fork()) {
		case -1:
			err(EXIT_FAILURE, "fork");
		case 0:
			off += job_len;
			break;
		default:
			child = true;
			px_n = off + job_len;
			goto process;
		}
	}

 process:
	for (size_t p = off; p < px_n; p++) {
		/* invert colors */
		ff_w[p].red   = UINT16_MAX - ff_r[p].red;
		ff_w[p].green = UINT16_MAX - ff_r[p].green;
		ff_w[p].blue  = UINT16_MAX - ff_r[p].blue;
		ff_w[p].alpha = ff_r[p].alpha;
	}

	if (!child) {
		int status;
		wait(&status);
		if (status != EXIT_SUCCESS)
			return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
