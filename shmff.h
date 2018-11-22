#ifndef _SHMFF_H_
#define _SHMFF_H_

#include <sys/time.h>

#include <stdbool.h>
#include <time.h>

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

int file2shm(const char *, struct shmff *, struct hdr **, struct px **);
void shm2file(const char *, struct hdr *, struct px *);
int shmff_read(struct shmff *, struct hdr **, struct px **);
int shmff_write(struct shmff *);
int shmff_free(struct shmff *, struct hdr *);

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
