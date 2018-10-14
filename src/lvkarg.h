#ifndef OSIC_LVKARG_H
#define OSIC_LVKARG_H

#include "lobject.h"

/*
 * variable keword argument
 * 'func(*foo);'
 * foo is iterable object for element like (keyword, argument)
 */
struct lvkarg {
	struct lobject object;

	struct lobject *arguments;
};

void *
lvkarg_create(struct osic *osic, struct lobject *arguments);

struct ltype *
lvkarg_type_create(struct osic *osic);

#endif /* osic_LVKARG_H */
