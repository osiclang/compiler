#ifndef OSIC_OEXCEPTION_H
#define OSIC_OEXCEPTION_H

#include "oObject.h"

struct oexception {
	struct oobject object;

	int throwed;
	struct oobject *clazz;
	struct oobject *message;

	int nframe;
	struct oobject *frame[128];
};

void *
oexception_create(struct osic *osic,
                  struct oobject *message);

struct otype *
oexception_type_create(struct osic *osic);

#endif /* OSIC_OEXCEPTION_H */
