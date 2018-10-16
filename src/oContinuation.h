#ifndef OSIC_OCONTINUATION_H
#define OSIC_OCONTINUATION_H

#include "oObject.h"

struct oframe;

struct ocontinuation {
	struct oobject object;

	int address;
	int framelen;
	int stacklen;

	struct oframe *pause;
	struct oframe **frame;
	struct oobject **stack;
	struct oobject *value;
};

void *
ocontinuation_create(struct osic *osic);

struct otype *
ocontinuation_type_create(struct osic *osic);

#endif /* OSIC_OCONTINUATION_H */
