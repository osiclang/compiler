#include "osic.h"
#include "larray.h"
#include "lsuper.h"
#include "lclass.h"
#include "lstring.h"
#include "linteger.h"
#include "linstance.h"

#include <stdio.h>
#include <string.h>

static struct lobject *
lsuper_call(struct osic *osic,
            struct lsuper *self,
            int argc, struct lobject *argv[])
{
	int i;
	struct lclass *clazz;
	struct lobject *base;
	struct lobject *item;
	struct lobject *zuper;

	base = NULL;
	clazz = ((struct linstance *)self->self)->clazz;
	if (argc) {
		zuper = argv[0];

		for (i = 0; i < larray_length(osic, clazz->bases); i++) {
			item = larray_get_item(osic, clazz->bases, i);
			if (lobject_is_equal(osic, zuper, item)) {
				base = item;

				break;
			}
		}
		if (!base) {
			return lobject_error_type(osic,
			                          "'%@' is not subclass of %@",
			                          self,
			                          zuper);
		}
	}
	self->base = base;
	osic_machine_push_object(osic, (struct lobject *)self);

	return osic->l_nil;
}

static struct lobject *
lsuper_get_attr(struct osic *osic,
                struct lsuper *self,
                struct lobject *name)
{
	int i;
	struct lclass *clazz;
	struct lobject *base;
	struct lobject *value;
	struct linstance *instance;

	value = NULL;
	instance = (struct linstance *)self->self;
	if (self->base) {
		base = self->base;
		if (!lobject_is_class(osic, base)) {
			base = instance->native;
		}
		value = lobject_get_attr(osic, base, name);
	} else {
		base = NULL;
		clazz = ((struct linstance *)self->self)->clazz;
		for (i = 0; i < larray_length(osic, clazz->bases); i++) {
			base = larray_get_item(osic, clazz->bases, i);
			if (!lobject_is_class(osic, base)) {
				base = instance->native;
			}
			value = lobject_get_attr(osic, base, name);
			if (value) {
				break;
			}
		}
	}

	if (value) {
		if (lobject_is_class(osic, base)) {
			value = lfunction_bind(osic, value, self->self);
		} else {
			base = lobject_method_call(osic,
			                           instance->native,
			                           LOBJECT_METHOD_SUPER,
			                           0,
			                           NULL);
			if (base) {
				value = lobject_get_attr(osic, base, name);
			}
		}
	}

	return value;
}

static struct lobject *
lsuper_string(struct osic *osic, struct lsuper *self)
{
	char buffer[9];

	snprintf(buffer, sizeof(buffer), "<super>");

	buffer[sizeof(buffer) - 1] = '\0';
	return lstring_create(osic, buffer, strlen(buffer));
}

static struct lobject *
lsuper_method(struct osic *osic,
                   struct lobject *self,
                   int method, int argc, struct lobject *argv[])
{
#define cast(a) ((struct lsuper *)(a))

	switch (method) {
	case LOBJECT_METHOD_GET_ATTR:
		return lsuper_get_attr(osic, cast(self), argv[0]);

	case LOBJECT_METHOD_CALL:
		if (!cast(self)->base) {
			return lsuper_call(osic, cast(self), argc, argv);
		}
		return NULL;

	case LOBJECT_METHOD_CALLABLE:
		if (!cast(self)->base) {
			return osic->l_true;
		}
		return osic->l_false;

	case LOBJECT_METHOD_STRING:
		return lsuper_string(osic, cast(self));

	default:
		return lobject_default(osic, self, method, argc, argv);
	}
}

void *
lsuper_create(struct osic *osic, struct lobject *binding)
{
	struct lsuper *self;

	self = lobject_create(osic, sizeof(*self), lsuper_method);
	if (self) {
		self->self = binding;
	}

	return self;
}

struct ltype *
lsuper_type_create(struct osic *osic)
{
	return ltype_create(osic, "super", lsuper_method, NULL);
}
