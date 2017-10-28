#ifndef _SHMFF_H_
#define _SHMFF_H_

struct shmff {
	char	magic[8];
	key_t	key;
	size_t	size;
};

struct hdr {
	char     magic[8];
	uint32_t width;
	uint32_t height;
};

struct px {
	uint16_t red;
	uint16_t green;
	uint16_t blue;
	uint16_t alpha;
};

int shmff_load(struct shmff *shmff, struct hdr **hdr, struct px **ff);
int shmff_free(struct shmff *shmff, struct hdr *hdr);

/* job control */

#define PARENT 0
#define CHILD  1

int fork_jobs(int jobs, size_t *off, size_t *px_n);
int catch_jobs(int jobs, int child);

/* debug */

#define debug(level, start, ...)					\
	do {								\
		if ((level) <= verbose) {				\
			struct timespec time;				\
			clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time);	\
			timespecsub(&time, &(start), &time);		\
			fprintf(stderr, "%2llu.%09lu ",			\
			    time.tv_sec, time.tv_nsec);			\
			fprintf(stderr, __VA_ARGS__);			\
			fputc('\n', stderr);				\
		}							\
	} while (false)


#endif
