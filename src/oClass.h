#ifndef OSIC_OCLASS_H
#define OSIC_OCLASS_H

#include "oObject.h"

struct oclass {
	struct oobject object;

	struct oobject *name;
	struct oobject *bases;

	struct oobject *attr;
	struct oobject *getter;
	struct oobject *setter;
};

void *
oclass_create(struct osic *osic,
              struct oobject *name,
              int nsupers,
              struct oobject *supers[],
              int nattrs,
              struct oobject *attrs[]);

struct otype *
oclass_type_create(struct osic *osic);

#endif /* OSIC_OCLASS_H */
