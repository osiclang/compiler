#ifndef OSIC_OKARG_H
#define OSIC_OKARG_H

#include "oObject.h"

/*
 * keyword argument
 * 'func(foo = 1);'
 */
struct okarg {
	struct oobject object;

	struct oobject *keyword;
	struct oobject *argument;
};

void *
okarg_create(struct osic *osic,
             struct oobject *keyword,
             struct oobject *argument);

struct otype *
okarg_type_create(struct osic *osic);

#endif /* OSIC_OKARG_H */
