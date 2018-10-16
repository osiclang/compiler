#ifndef OSIC_OVARG_H
#define OSIC_OVARG_H

#include "oObject.h"

/*
 * variable argument
 * 'func(*foo);'
 * foo is iterable object
 */
struct ovarg {
	struct oobject object;

	struct oobject *arguments;
};

void *
ovarg_create(struct osic *osic, struct oobject *arguments);

struct otype *
ovarg_type_create(struct osic *osic);

#endif /* OSIC_OVARG_H */
