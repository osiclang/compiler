#ifndef OSIC_OSENTINEL_H
#define OSIC_OSENTINEL_H

#include "oObject.h"

struct osentinel {
	struct oobject object;
};

void *
osentinel_create(struct osic *);

#endif /* OSIC_OSENTINEL_H */
