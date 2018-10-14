#ifndef OSIC_LVARG_H
#define OSIC_LVARG_H

#include "lobject.h"

/*
 * variable argument
 * 'func(*foo);'
 * foo is iterable object
 */
struct lvarg {
	struct lobject object;

	struct lobject *arguments;
};

void *
lvarg_create(struct osic *osic, struct lobject *arguments);

struct ltype *
lvarg_type_create(struct osic *osic);

#endif /* osic_LVARG_H */
