#ifndef OSIC_LCONTINUATION_H
#define OSIC_LCONTINUATION_H

#include "lobject.h"

struct lframe;

struct lcontinuation {
	struct lobject object;

	int address;
	int framelen;
	int stacklen;

	struct lframe *pause;
	struct lframe **frame;
	struct lobject **stack;
	struct lobject *value;
};

void *
lcontinuation_create(struct osic *osic);

struct ltype *
lcontinuation_type_create(struct osic *osic);

#endif /* osic_LCONTINUATION_H */
