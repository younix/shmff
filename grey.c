#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>

#include <err.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "shmff.h"

static int verbose = 0;
static struct timespec start;

void
usage(void)
{
	fputs("grey [-v] [-j jobs]\n", stderr);
	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	struct shmff shmff;
	struct hdr *hdr;
	struct px *ff;
	unsigned int jobs = 1;
	int ch;

	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);

	while ((ch = getopt(argc, argv, "j:vh")) != -1) {
		switch (ch) {
		case 'j':
			errno = 0;
			if ((jobs = strtoul(optarg, NULL, 0)) == 0) {
				if (errno != 0)
					err(EXIT_FAILURE, "strtoul");
				errx(EXIT_FAILURE, "invalid jobs");
			}
			break;
		case 'v':
			verbose++;
			break;
		case 'h':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	/* read shmff structure from stdin */
	if (shmff_read(&shmff, &hdr, &ff) == -1)
		err(EXIT_FAILURE, "shmff_read");

	debug(1, start, "start %d jobs", jobs);

	size_t px_n = hdr->width * hdr->height;
	size_t off = 0;
	int child = fork_jobs(jobs, &off, &px_n);

	for (size_t p = off; p < px_n; p++) {
		/* convert pixel to grey */
		ff[p].red = ff[p].green = ff[p].blue =
		   (ff[p].red   * 30 +
		    ff[p].green * 59 +
		    ff[p].blue  * 11) / 100;

		ff[p].alpha = ff[p].alpha;
	}

	if (catch_jobs(jobs, child) != EXIT_SUCCESS)
		goto err;

	/* write shmff structure to stdout */
	if (child == 0) {
		debug(1, start, "write shm info to stdout");
		if (fwrite(&shmff, sizeof shmff, 1, stdout) < 1) {
			perror("fwrite");
			goto err;
		}
	}

	return EXIT_SUCCESS;
 err:
	if (shmff_free(&shmff, hdr) == -1)
		perror("shmff_free");

	return EXIT_FAILURE;
}
