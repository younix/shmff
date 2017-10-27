#ifndef FF_H
#define FF_H

/*
 * farbfeld
 * sharedff
 */

#define PARENT 0
#define CHILD  1

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

struct shmff {
	char	magic[8];
	key_t	key;
	size_t	size;
};

#endif
