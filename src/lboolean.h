#ifndef OSIC_LBOOLEAN_H
#define OSIC_LBOOLEAN_H

#include "lobject.h"

struct lboolean {
	struct lobject object;
};

void *
lboolean_create(struct osic *osic, int value);

struct ltype *
lboolean_type_create(struct osic *osic);

#endif /* osic_LBOOLEAN_H */
