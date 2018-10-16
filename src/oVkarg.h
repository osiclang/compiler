#ifndef OSIC_OVKARG_H
#define OSIC_OVKARG_H

#include "oObject.h"

/*
 * variable keword argument
 * 'func(*foo);'
 * foo is iterable object for element like (keyword, argument)
 */
struct ovkarg {
	struct oobject object;

	struct oobject *arguments;
};

void *
ovkarg_create(struct osic *osic, struct oobject *arguments);

struct otype *
ovkarg_type_create(struct osic *osic);

#endif /* OSIC_OVKARG_H */
