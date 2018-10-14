#ifndef OSIC_LARRAY_H
#define OSIC_LARRAY_H

#include "lobject.h"

struct larray {
	struct lobject object;

	int alloc;
	int count;
	struct lobject **items;
};

struct lobject *
larray_append(struct osic *osic,
              struct lobject *self,
              int argc, struct lobject *argv[]);

struct lobject *
larray_get_item(struct osic *osic,
                struct lobject *self,
                long i);

struct lobject *
larray_set_item(struct osic *osic,
                struct lobject *self,
                long i,
                struct lobject *value);

struct lobject *
larray_del_item(struct osic *osic,
                struct lobject *self,
                long i);

long
larray_length(struct osic *osic, struct lobject *self);

void *
larray_create(struct osic *osic, int count, struct lobject *items[]);

struct ltype *
larray_type_create(struct osic *osic);

#endif /* osic_LARRAY_H */
