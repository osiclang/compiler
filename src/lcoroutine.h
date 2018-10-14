#ifndef OSIC_LCOROUTINE_H
#define OSIC_LCOROUTINE_H

#include "lobject.h"

struct lframe;

struct lcoroutine {
	struct lobject object;

	struct lframe *frame;

	int address;
	int stacklen;
	int finished;

	struct lobject *current;
	struct lobject **stack;
};

void *
lcoroutine_create(struct osic *osic, struct lframe *frame);

struct ltype *
lcoroutine_type_create(struct osic *osic);

#endif /* osic_LCOROUTINE_H */
