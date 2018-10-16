#ifndef OSIC_OTYPE_H
#define OSIC_OTYPE_H

#include "oObject.h"

/*
 * three methods:
 * 1, `otype->object->method' used for identity type's type
 * 2, `otype->method' used for identity type's object's type
 * 3, `otype->type_method' actual type object's method
 */
struct otype {
	struct oobject object;

	const char *name;
	oobject_method_t method;      /* method of object */
	oobject_method_t type_method; /* method of type   */
};

void *
otype_create(struct osic *osic,
             const char *name,
             oobject_method_t method,
             oobject_method_t type_method);

struct otype *
otype_type_create(struct osic *osic);

#endif /* OSIC_OTYPE_H */
