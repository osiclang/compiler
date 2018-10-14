#ifndef OSIC_LNUMBER_H
#define OSIC_LNUMBER_H

#include "lobject.h"

struct lnumber {
	struct lobject object;

	double value;
};

double
lnumber_to_double(struct osic *osic, struct lobject *self);

void *
lnumber_create_from_long(struct osic *osic, long value);

void *
lnumber_create_from_cstr(struct osic *osic, const char *value);

struct ltype *
lnumber_type_create(struct osic *osic);

#endif /* osic_LNUMBER_H */
