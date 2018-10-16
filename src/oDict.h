#ifndef OSIC_ODICT_H
#define OSIC_ODICT_H

#include "oObject.h"

struct odict {
	struct oobject object;

	struct oobject *table;
};

void *
odict_create(struct osic *osic, int count, struct oobject *items[]);

struct otype *
odict_type_create(struct osic *osic);

#endif /* OSIC_ODICT_H */
