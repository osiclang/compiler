#ifndef OSIC_LNIL_H
#define OSIC_LNIL_H

#include "lobject.h"

struct lnil {
	struct lobject object;
};

void *
lnil_create(struct osic *);

#endif /* osic_LNIL_H */
