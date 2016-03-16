#ifndef FF_H
#define FF_H

struct hdr {
	char		magic[8];
	uint32_t	width;
	uint32_t	height;
};

struct px {
	uint16_t red;
	uint16_t green;
	uint16_t blue;
	uint16_t alpha;
};

struct shm_ff {
	uint32_t width;
	uint32_t height;
	size_t size;
	key_t key;
};

#endif
