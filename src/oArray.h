#ifndef OSIC_OARRAY_H
#define OSIC_OARRAY_H

#include "oObject.h"

struct oarray {
	struct oobject object;

	int alloc;
	int count;
	struct oobject **items;
};

struct oobject *
oarray_append(struct osic *osic,
              struct oobject *self,
              int argc, struct oobject *argv[]);

struct oobject *
oarray_get_item(struct osic *osic,
                struct oobject *self,
                long i);

struct oobject *
oarray_set_item(struct osic *osic,
                struct oobject *self,
                long i,
                struct oobject *value);

struct oobject *
oarray_del_item(struct osic *osic,
                struct oobject *self,
                long i);

long
oarray_length(struct osic *osic, struct oobject *self);

void *
oarray_create(struct osic *osic, int count, struct oobject *items[]);

struct otype *
oarray_type_create(struct osic *osic);

#endif /* OSIC_OARRAY_H */
