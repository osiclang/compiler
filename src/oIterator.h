#ifndef OSIC_OITERATOR_H
#define OSIC_OITERATOR_H

#include "oObject.h"

typedef struct oobject *(*oiterator_next_t)(struct osic *,
                                            struct oobject *,
                                            struct oobject **);

struct oiterator {
	struct oobject object;

	struct oobject *iterable;
	struct oobject *context;

	long max;
	oiterator_next_t next;
};

struct oobject *
oiterator_to_array(struct osic *osic, struct oobject *iterator, long max);

void *
oiterator_create(struct osic *osic,
                 struct oobject *iterable,
                 struct oobject *context,
                 oiterator_next_t next);

struct otype *
oiterator_type_create(struct osic *osic);

#endif /* OSIC_OITERATOR_H */
