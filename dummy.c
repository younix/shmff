#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <err.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ff.h"
#include "libshmff.h"

int
main(void)
{
	struct px *ff_r;
	struct px *ff_w;
	struct hdr *hdr_r;
	struct hdr *hdr_w;

	setshmff(&hdr_r, &ff_r);
	setshmff(&hdr_w, &ff_w);

	memmove(hdr_r, hdr_r,
	    sizeof(*hdr_r) + sizeof(*ff_r) * hdr_r->width * hdr_r->height);

	return EXIT_SUCCESS;
}
