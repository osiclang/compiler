#ifndef OSIC_OMODULE_H
#define OSIC_OMODULE_H

#include "oObject.h"

struct omodule {
	struct oobject object;

	int nlocals;
	struct oobject *name;
	struct oframe *frame;

	struct oobject *attr;
	struct oobject *getter;
	struct oobject *setter;
};

void *
omodule_create(struct osic *osic, struct oobject *name);

struct otype *
omodule_type_create(struct osic *osic);

#endif /* OSIC_OMODULE_H */
