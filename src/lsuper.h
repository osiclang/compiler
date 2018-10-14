#ifndef OSIC_LSUPER_H
#define OSIC_LSUPER_H

#include "lobject.h"

struct lsuper {
	struct lobject object;

	struct lobject *self;
	struct lobject *base;
};

void *
lsuper_create(struct osic *osic, struct lobject *binding);

struct ltype *
lsuper_type_create(struct osic *osic);

#endif /* osic_LSUPER_H */
