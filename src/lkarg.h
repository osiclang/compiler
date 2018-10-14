#ifndef OSIC_LKARG_H
#define OSIC_LKARG_H

#include "lobject.h"

/*
 * keyword argument
 * 'func(foo = 1);'
 */
struct lkarg {
	struct lobject object;

	struct lobject *keyword;
	struct lobject *argument;
};

void *
lkarg_create(struct osic *osic,
             struct lobject *keyword,
             struct lobject *argument);

struct ltype *
lkarg_type_create(struct osic *osic);

#endif /* osic_LKARG_H */
