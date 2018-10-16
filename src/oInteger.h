#ifndef OSIC_OINTEGER_H
#define OSIC_OINTEGER_H

#include "extend.h"

struct ointeger {
	struct oobject object;

	int sign;
	int length;
	int ndigits;
	extend_t digits[1];
};

struct oobject *
ointeger_method(struct osic *osic,
                struct oobject *self,
                int method, int argc, struct oobject *argv[]);

long
ointeger_to_long(struct osic *osic, struct oobject *self);

void *
ointeger_create_from_long(struct osic *osic, long value);

void *
ointeger_create_from_cstr(struct osic *osic, const char *cstr);

struct otype *
ointeger_type_create(struct osic *osic);

#endif /* OSIC_OINTEGER_H */
