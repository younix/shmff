#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>

#include <err.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ff.h"
#include "libshmff.h"

void
usage(void)
{
	fputs("grey [-j jobs]\n", stderr);
	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	struct shmff shmff_r;
	struct shmff shmff_w;
	struct hdr *hdr_r = NULL;
	struct hdr *hdr_w = NULL;
	struct px *ff_r = NULL;
	struct px *ff_w = NULL;
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

	setshmff(&hdr_r, &hdr_w, &shmff_r, &shmff_w, &ff_r, &ff_w);

	size_t px_n = hdr_r->width * hdr_r->height;
	size_t off = 0;
	int child = fork_jobs(jobs, &off, &px_n);

	for (size_t p = 0; p < px_n; p++) {
		/* convert pixel to grey */
		ff_w[p].red = ff_w[p].green = ff_w[p].blue =
		   (ff_r[p].red   * 30 +
		    ff_w[p].green * 59 +
		    ff_r[p].blue  * 11) / 100;
		ff_w[p].alpha = ff_r[p].alpha;
	}

	if (child == 0) {
		int status;
		wait(&status);
		if (status != EXIT_SUCCESS)
			return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
