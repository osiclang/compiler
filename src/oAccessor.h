#ifndef OSIC_OACCESSOR_H
#define OSIC_OACCESSOR_H

#include "oObject.h"

struct oaccessor {
	struct oobject object;

	struct oobject *self;

	int count;
	struct oobject *items[1];
};

void *
oaccessor_create(struct osic *osic, int count, struct oobject *items[]);

#endif /* OSIC_OACCESSOR_H */
