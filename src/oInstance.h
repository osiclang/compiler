#ifndef OSIC_OINSTANCE_H
#define OSIC_OINSTANCE_H

#include "oObject.h"

struct oinstance {
	struct oobject object;

	struct oclass *clazz;
	struct oobject *attr;
	struct oobject *native;
};

void *
oinstance_create(struct osic *osic, struct oclass *clazz);

struct otype *
oinstance_type_create(struct osic *osic);

#endif /* OSIC_OINSTANCE_H */
