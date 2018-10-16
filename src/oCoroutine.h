#ifndef OSIC_OCOROUTINE_H
#define OSIC_OCOROUTINE_H

#include "oObject.h"

struct oframe;

struct ocoroutine {
	struct oobject object;

	struct oframe *frame;

	int address;
	int stacklen;
	int finished;

	struct oobject *current;
	struct oobject **stack;
};

void *
ocoroutine_create(struct osic *osic, struct oframe *frame);

struct otype *
ocoroutine_type_create(struct osic *osic);

#endif /* OSIC_OCOROUTINE_H */
