#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <err.h>
#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ff.h"

#define KEY_ID_READ  0	/* read shm id */
#define KEY_ID_WRITE 1	/* write shm id */

bool benchmark = false;

void
signal_handler(int sig)
{
	if (sig == SIGHUP)
		benchmark = false;
}

void
convert(const char *prog, struct shmff *shmff_r, struct shmff *shmff_w)
{
	FILE *ph;

	if ((ph = popen(prog, "w")) == NULL)
		err(EXIT_FAILURE, "popen");

	if (fwrite(shmff_r, sizeof *shmff_r, 1, ph) < 1)
		err(EXIT_FAILURE, "fwrite");
	if (fwrite(shmff_w, sizeof *shmff_w, 1, ph) < 1)
		err(EXIT_FAILURE, "fwrite");

	if (pclose(ph) != EXIT_SUCCESS)
		err(EXIT_FAILURE, "pclose");
}

struct shmff *
convert_cmds(struct shmff *shmff_one, struct shmff *shmff_two)
{
	char cmd[BUFSIZ];
	struct shmff *shmff_tmp;

	while (fgets(cmd, sizeof cmd, stdin) != NULL) {
		convert(cmd, shmff_one, shmff_two);

		/* swap pages */
		shmff_tmp = shmff_one;
		shmff_one = shmff_two;
		shmff_two = shmff_tmp;
	}

	return shmff_one;
}

void
benchmark_cmd(const char *cmd, struct shmff *shmff_r, struct shmff *shmff_w)
{
	uint64_t benchmark_counter;
	time_t start_time, end_time;

	if ((start_time = time(NULL)) == -1)
		err(EXIT_FAILURE, "time");

	for (benchmark_counter = 0; benchmark; benchmark_counter++)
		convert(cmd, shmff_r, shmff_w);

	if ((end_time = time(NULL)) == -1)
		err(EXIT_FAILURE, "time");

	fprintf(stderr, "%" PRId64 " rounds\n", benchmark_counter);
	fprintf(stderr, "%" PRId64 " sec\n", end_time - start_time);
}

void
shm2file(const char *file, struct hdr *hdr, struct px *ff)
{
	FILE *fh = stdout;
	size_t px_n = hdr->width * hdr->height;

	if (file != NULL) {
		if ((fh = fopen(file, "w")) == NULL)
			err(EXIT_FAILURE, "fopen");
	}

	hdr->width = htonl(hdr->width);
	hdr->height = htonl(hdr->height);

	/* save header */
	if (fwrite(hdr, sizeof *hdr, 1, fh) < 1)
		err(EXIT_FAILURE, "fread");

	for (size_t p = 0; p < px_n; p++) {
		/* convert net endian to host endian */
		ff[p].red   = htons(ff[p].red);
		ff[p].green = htons(ff[p].green);
		ff[p].blue  = htons(ff[p].blue);
		ff[p].alpha = htons(ff[p].alpha);

		if (fwrite(&ff[p], sizeof *ff, 1, fh) < 1)
			err(EXIT_FAILURE, "unexpected end-of-file");
	}

	if (fclose(fh) == EOF)
		err(EXIT_FAILURE, "fclose");
}

void
usage(void)
{
	fputs("shmff [-hb] [-W width] [-H height] <in file> <out file>"
	    " [algorithm]\n", stderr);
	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	int shmid_r;
	int shmid_w;
	int ch;
	key_t key_r;	/* read page */
	key_t key_w;	/* write page */
	FILE *fh = stdin;
	char *file_in = NULL;
	char *file_out = NULL;
	size_t width = 0;
	size_t height = 0;

	struct hdr hdr;
	struct hdr *hdr_r;
	struct hdr *hdr_w;
	struct px *ff_r = NULL;
	struct px *ff_w = NULL;

	while ((ch = getopt(argc, argv, "bW:H:h")) != -1) {
		switch (ch) {
		case 'b':
			benchmark = true;
			break;
		case 'W':
			if ((width = strtoul(optarg, NULL, 0)) == 0) {
				if (errno != 0)
					err(EXIT_FAILURE, "strtoul");
				errx(EXIT_FAILURE, "invailed width");
			}
			break;
		case 'H':
			if ((height = strtoul(optarg, NULL, 0)) == 0) {
				if (errno != 0)
					err(EXIT_FAILURE, "strtoul");
				errx(EXIT_FAILURE, "invailed height");
			}
			break;
		case 'h':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc < 2)
		usage();

	file_in = argv[0];
	file_out = argv[1];

	if (benchmark)
		signal(SIGHUP, signal_handler);

	if ((fh = fopen(file_in, "r")) == NULL)
		err(EXIT_FAILURE, "fopen");

	/* read header */
	if (fread(&hdr, sizeof hdr, 1, fh) < 1)
		err(EXIT_FAILURE, "fread");

	/* check magic string */
	if (memcmp(hdr.magic, "farbfeld", sizeof(hdr.magic)) != 0)
		err(EXIT_FAILURE, "file is not in ff format");

	hdr.width = ntohl(hdr.width);
	hdr.height = ntohl(hdr.height);

	size_t px_n = hdr.width * hdr.height;
	size_t ff_size = sizeof(hdr) + hdr.width * hdr.height * sizeof(struct px);

	/* generate name of shm segment by file path */
	key_r = ftok(file_in, KEY_ID_READ);
	key_w = ftok(file_in, KEY_ID_WRITE);

	/* Create and attach the shm segment for reading.  */
	if ((shmid_r = shmget(key_r, ff_size, IPC_CREAT | 0600)) < 0)
		err(EXIT_FAILURE, "shmget 1");
	if ((hdr_r = shmat(shmid_r, NULL, 0)) == (void *) -1)
		err(EXIT_FAILURE, "shmat");

	/* Create and attach the shm segment for writing.  */
	if ((shmid_w = shmget(key_w, ff_size, IPC_CREAT | 0600)) < 0)
		err(EXIT_FAILURE, "shmget 2");
	if ((hdr_w = shmat(shmid_w, NULL, 0)) == (void *) -1)
		err(EXIT_FAILURE, "shmat");

	memcpy(hdr_r, &hdr, sizeof hdr);

	ff_r = (struct px *)(hdr_r + 1);
	ff_w = (struct px *)(hdr_w + 1);

	/* read image into memory */
	for (size_t p = 0; p < px_n; p++) {
		if (fread(&ff_r[p], sizeof *ff_r, 1, fh) < 1)
			err(EXIT_FAILURE, "unexpected end-of-file");

		/* convert net endian to host endian */
		ff_r[p].red   = ntohs(ff_r[p].red);
		ff_r[p].green = ntohs(ff_r[p].green);
		ff_r[p].blue  = ntohs(ff_r[p].blue);
		ff_r[p].alpha = ntohs(ff_r[p].alpha);
	}

	if (fclose(fh) == EOF)
		err(EXIT_FAILURE, "fclose");

	struct shmff shmff_r;
	struct shmff shmff_w;

	shmff_r.size = shmff_w.size = ff_size;
	shmff_r.key = key_r;
	shmff_w.key = key_w;

	if (benchmark) {
		if (argc < 3)
			usage();
		benchmark_cmd(argv[2], &shmff_r, &shmff_w);
		shm2file(file_out, hdr_w, ff_w);
	} else {
		if (convert_cmds(&shmff_r, &shmff_w) == &shmff_w) {
			shm2file(file_out, hdr_w, ff_w);
		} else {
			shm2file(file_out, hdr_r, ff_r);
		}
	}

	/* unmap */
	if (shmdt(hdr_r) == -1)
		err(EXIT_FAILURE, "shmdt");
	if (shmdt(hdr_w) == -1)
		err(EXIT_FAILURE, "shmdt");

	if (shmctl(shmid_r, IPC_RMID, NULL) == -1)
		err(EXIT_FAILURE, "shmctl");
	if (shmctl(shmid_w, IPC_RMID, NULL) == -1)
		err(EXIT_FAILURE, "shmctl");

	return EXIT_SUCCESS;
}
