#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <err.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ff.h"

int
main(void)
{
	struct shm_ff shmff_r;
	struct shm_ff shmff_w;
	struct px *ff_r;
	struct px *ff_w;
	int shmid;

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

	memmove(ff_w, ff_r, shmff_r.size);

	return EXIT_SUCCESS;
}
