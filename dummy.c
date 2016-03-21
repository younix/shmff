#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <err.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ff.h"
#include "libshmff.h"

int
main(void)
{
	struct shmff shmff_r;
	struct shmff shmff_w;
	struct px *ff_r;
	struct px *ff_w;
	struct hdr *hdr_r;
	struct hdr *hdr_w;

	setshmff(&hdr_r, &hdr_w, &shmff_r, &shmff_w, &ff_r, &ff_w);

	memmove(ff_w, ff_r, shmff_r.size);

	return EXIT_SUCCESS;
}
