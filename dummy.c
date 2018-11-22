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

	for (;;) {
		/* read shmff structure from stdin */
		if (shmff_read(&shmff, &hdr, &ff) == -1) {
			if (feof(stdin))
				break;
			err(EXIT_FAILURE, "shmff_read");
		}

		/* do some stuff with the pixels in ff ... */

		/* write shmff structure to stdout */
		if (shmff_write(&shmff) == -1)
			goto err;
	}

	return EXIT_SUCCESS;
 err:
	if (shmff_free(&shmff, hdr) == -1)
		perror("shmff_free");
	return EXIT_FAILURE;
}
