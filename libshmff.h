#ifndef LIBSHMFF_H
#define LIBSHMFF_H

#include "ff.h"

void setshmff(struct shmff *shmff, struct hdr **hdr, struct px **ff);
int fork_jobs(int jobs, size_t *off, size_t *px_n);
int catch_jobs(int jobs, int child);

#endif
