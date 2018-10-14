#ifndef OSIC_LEXCEPTION_H
#define OSIC_LEXCEPTION_H

#include "lobject.h"

struct lexception {
	struct lobject object;

	int throwed;
	struct lobject *clazz;
	struct lobject *message;

	int nframe;
	struct lobject *frame[128];
};

void *
lexception_create(struct osic *osic,
                  struct lobject *message);

struct ltype *
lexception_type_create(struct osic *osic);

#endif /* osic_LEXCEPTION_H */
