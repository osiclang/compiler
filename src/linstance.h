#ifndef OSIC_LINSTANCE_H
#define OSIC_LINSTANCE_H

#include "lobject.h"

struct linstance {
	struct lobject object;

	struct lclass *clazz;
	struct lobject *attr;
	struct lobject *native;
};

void *
linstance_create(struct osic *osic, struct lclass *clazz);

struct ltype *
linstance_type_create(struct osic *osic);

#endif /* osic_LINSTANCE_H */
