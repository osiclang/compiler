#ifndef OSIC_LACCESSOR_H
#define OSIC_LACCESSOR_H

#include "lobject.h"

struct laccessor {
	struct lobject object;

	struct lobject *self;

	int count;
	struct lobject *items[1];
};

void *
laccessor_create(struct osic *osic, int count, struct lobject *items[]);

#endif /* osic_LACCESSOR_H */
