#ifndef OSIC_ONIL_H
#define OSIC_ONIL_H

#include "oObject.h"

struct onil {
	struct oobject object;
};

void *
onil_create(struct osic *);

#endif /* OSIC_ONIL_H */
