#ifndef OSIC_LMODULE_H
#define OSIC_LMODULE_H

#include "lobject.h"

struct lmodule {
	struct lobject object;

	int nlocals;
	struct lobject *name;
	struct lframe *frame;

	struct lobject *attr;
	struct lobject *getter;
	struct lobject *setter;
};

void *
lmodule_create(struct osic *osic, struct lobject *name);

struct ltype *
lmodule_type_create(struct osic *osic);

#endif /* osic_LMODULE_H */
