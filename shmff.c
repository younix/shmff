#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <err.h>
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
convert(const char *prog, struct shm_ff *shmff_r, struct shm_ff *shmff_w)
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

void
convert_cmds(struct shm_ff *shmff_one, struct shm_ff *shmff_two)
{
	char cmd[BUFSIZ];
	struct shm_ff *shmff_tmp;

	while (fgets(cmd, sizeof cmd, stdin) != NULL) {
		convert(cmd, shmff_one, shmff_two);

		/* swap pages */
		shmff_tmp = shmff_one;
		shmff_one = shmff_two;
		shmff_two = shmff_tmp;
	}
}

void
benchmark_cmd(const char *cmd, struct shm_ff *shmff_r, struct shm_ff *shmff_w)
{
	uint64_t benchmark_counter;

	for (benchmark_counter = 0; benchmark; benchmark_counter++)
		convert(cmd, shmff_r, shmff_w);

	fprintf(stderr, "%" PRId64 "\n", benchmark_counter);
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
	fputs("shmff [-hb] <in file> <out file> [algorithm]\n", stderr);
	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	int shmid;
	int ch;
	key_t key_r;	/* read page */
	key_t key_w;	/* write page */
	FILE *fh = stdin;
	char *file_in = NULL;
	char *file_out = NULL;

	struct hdr hdr;
	struct px *ff_r = NULL;
	struct px *ff_w = NULL;

	while ((ch = getopt(argc, argv, "bh")) != -1) {
		switch (ch) {
		case 'b':
			benchmark = true;
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
	size_t ff_size = hdr.width * hdr.height * sizeof(struct px);

	/* generate name of shm segment by file path */
	key_r = ftok(file_in, KEY_ID_READ);
	key_w = ftok(file_in, KEY_ID_WRITE);

	/* Create and attach the shm segment for reading.  */
	if ((shmid = shmget(key_r, ff_size, IPC_CREAT | 0666)) < 0)
		err(EXIT_FAILURE, "shmget 1");
	if ((ff_r = shmat(shmid, NULL, 0)) == (void *) -1)
		err(EXIT_FAILURE, "shmat");

	/* Create and attach the shm segment for writing.  */
	if ((shmid = shmget(key_w, ff_size, IPC_CREAT | 0666)) < 0)
		err(EXIT_FAILURE, "shmget 2");
	if ((ff_w = shmat(shmid, NULL, 0)) == (void *) -1)
		err(EXIT_FAILURE, "shmat");

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

	struct shm_ff shmff_r;
	struct shm_ff shmff_w;

	shmff_r.width  = shmff_w.width  = hdr.width;
	shmff_r.height = shmff_w.height = hdr.height;
	shmff_r.size   = shmff_w.size   = ff_size;
	shmff_r.key = key_r;
	shmff_w.key = key_w;

	if (benchmark) {
		if (argc < 3)
			usage();
		benchmark_cmd(argv[2], &shmff_r, &shmff_w);
	} else {
		convert_cmds(&shmff_r, &shmff_w);
	}

	shm2file(file_out, &hdr, ff_r);

	return EXIT_SUCCESS;
}
