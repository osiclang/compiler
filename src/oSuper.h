#ifndef OSIC_OSUPER_H
#define OSIC_OSUPER_H

#include "oObject.h"

struct osuper {
	struct oobject object;

	struct oobject *self;
	struct oobject *base;
};

void *
osuper_create(struct osic *osic, struct oobject *binding);

struct otype *
osuper_type_create(struct osic *osic);

#endif /* OSIC_OSUPER_H */
