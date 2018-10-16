#ifndef OSIC_OBOLLEAN_H
#define OSIC_OBOLLEAN_H

#include "oObject.h"

struct oboolean {
	struct oobject object;
};

void *
oboolean_create(struct osic *osic, int value);

struct otype *
oboolean_type_create(struct osic *osic);

#endif /* OSIC_OBOLLEAN_H */
