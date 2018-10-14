#ifndef OSIC_LITERATOR_H
#define OSIC_LITERATOR_H

#include "lobject.h"

typedef struct lobject *(*literator_next_t)(struct osic *,
                                            struct lobject *,
                                            struct lobject **);

struct literator {
	struct lobject object;

	struct lobject *iterable;
	struct lobject *context;

	long max;
	literator_next_t next;
};

struct lobject *
literator_to_array(struct osic *osic, struct lobject *iterator, long max);

void *
literator_create(struct osic *osic,
                 struct lobject *iterable,
                 struct lobject *context,
                 literator_next_t next);

struct ltype *
literator_type_create(struct osic *osic);

#endif /* osic_LITERATOR_H */
