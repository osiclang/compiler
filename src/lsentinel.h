#ifndef OSIC_LSENTINEL_H
#define OSIC_LSENTINEL_H

#include "lobject.h"

struct lsentinel {
	struct lobject object;
};

void *
lsentinel_create(struct osic *);

#endif /* osic_LSENTINEL_H */
