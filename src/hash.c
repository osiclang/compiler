#include "osic.h"
#include "hash.h"

#include <stdint.h>

uint32_t
djb_hash(const void *key, long len, long seed)
{
	int i;
	unsigned int h;
	const unsigned char *p;

	p = key;
	h = ((unsigned int)seed & 0xffffffff);
	for (i = 0; i < len; i++) {
		h = (unsigned int)(33 * (long)h ^ p[i]);
	}

	return h;
}

long
osic_hash(struct osic *osic, const void *key, long len)
{
	return djb_hash(key, len, osic->l_random);
}
