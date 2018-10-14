#ifndef OSIC_LSTRING_H
#define OSIC_LSTRING_H

#include "lobject.h"

struct lstring {
	struct lobject object;

	long length;

	/* lstring is dynamic size */
	char buffer[1];
};

const char *
lstring_to_cstr(struct osic *osic, struct lobject *object);

char *
lstring_buffer(struct osic *osic, struct lobject *object);

long
lstring_length(struct osic *osic, struct lobject *object);

void *
lstring_create(struct osic *osic, const char *buffer, long length);

struct ltype *
lstring_type_create(struct osic *);

#endif /* osic_LSTRING_H */
