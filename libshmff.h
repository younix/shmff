#ifndef LIBSHMFF_H
#define LIBSHMFF_H

#include "ff.h"

void setshmff(struct hdr **hdr_r, struct hdr **hdr_w, struct shmff *shmff_r,
    struct shmff *shmff_w, struct px **ff_r, struct px **ff_w);

int fork_jobs(int jobs, size_t *off, size_t *px_n);

#endif
