#ifndef OSIC_ONUMBER_H
#define OSIC_ONUMBER_H

#include "oObject.h"

struct onumber {
	struct oobject object;

	double value;
};

double
onumber_to_double(struct osic *osic, struct oobject *self);

void *
onumber_create_from_long(struct osic *osic, long value);

void *
onumber_create_from_cstr(struct osic *osic, const char *value);

struct otype *
onumber_type_create(struct osic *osic);

#endif /* OSIC_ONUMBER_H */
