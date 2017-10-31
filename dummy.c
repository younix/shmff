#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "shmff.h"

int
main(void)
{
	struct shmff shmff;
	struct hdr *hdr;
	struct px *ff;

	/* read shmff structure from stdin */
	if (shmff_load(&shmff, &hdr, &ff) == -1)
		err(EXIT_FAILURE, "shmff_load");

	/* do some stuff with the pixels in ff ... */

	/* write shmff structure to stdout */
	if (fwrite(&shmff, sizeof shmff, 1, stdout) < 1) {
		perror("fwrite");
		goto err;
	}

	return EXIT_SUCCESS;
 err:
	if (shmff_free(&shmff, hdr) == -1)
		perror("shmff_free");
	return EXIT_FAILURE;
}
