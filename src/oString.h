#ifndef OSIC_OSTRING_H
#define OSIC_OSTRING_H

#include "oObject.h"

struct ostring {
	struct oobject object;

	long length;

	/* ostring is dynamic size */
	char buffer[1];
};

const char *
ostring_to_cstr(struct osic *osic, struct oobject *object);

char *
ostring_buffer(struct osic *osic, struct oobject *object);

long
ostring_length(struct osic *osic, struct oobject *object);

void *
ostring_create(struct osic *osic, const char *buffer, long length);

struct otype *
ostring_type_create(struct osic *);

#endif /* OSIC_OSTRING_H */
