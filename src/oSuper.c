#include "osic.h"
#include "oArray.h"
#include "oSuper.h"
#include "oClass.h"
#include "oString.h"
#include "oInteger.h"
#include "oInstance.h"

#include <stdio.h>
#include <string.h>

static struct oobject *
osuper_call(struct osic *osic,
            struct osuper *self,
            int argc, struct oobject *argv[])
{
	int i;
	struct oclass *clazz;
	struct oobject *base;
	struct oobject *item;
	struct oobject *zuper;

	base = NULL;
	clazz = ((struct oinstance *)self->self)->clazz;
	if (argc) {
		zuper = argv[0];

		for (i = 0; i < oarray_length(osic, clazz->bases); i++) {
			item = oarray_get_item(osic, clazz->bases, i);
			if (oobject_is_equal(osic, zuper, item)) {
				base = item;

				break;
			}
		}
		if (!base) {
			return oobject_error_type(osic,
			                          "'%@' is not subclass of %@",
			                          self,
			                          zuper);
		}
	}
	self->base = base;
	osic_machine_push_object(osic, (struct oobject *)self);

	return osic->l_nil;
}

static struct oobject *
osuper_get_attr(struct osic *osic,
                struct osuper *self,
                struct oobject *name)
{
	int i;
	struct oclass *clazz;
	struct oobject *base;
	struct oobject *value;
	struct oinstance *instance;

	value = NULL;
	instance = (struct oinstance *)self->self;
	if (self->base) {
		base = self->base;
		if (!oobject_is_class(osic, base)) {
			base = instance->native;
		}
		value = oobject_get_attr(osic, base, name);
	} else {
		base = NULL;
		clazz = ((struct oinstance *)self->self)->clazz;
		for (i = 0; i < oarray_length(osic, clazz->bases); i++) {
			base = oarray_get_item(osic, clazz->bases, i);
			if (!oobject_is_class(osic, base)) {
				base = instance->native;
			}
			value = oobject_get_attr(osic, base, name);
			if (value) {
				break;
			}
		}
	}

	if (value) {
		if (oobject_is_class(osic, base)) {
			value = ofunction_bind(osic, value, self->self);
		} else {
			base = oobject_method_call(osic,
			                           instance->native,
			                           OOBJECT_METHOD_SUPER,
			                           0,
			                           NULL);
			if (base) {
				value = oobject_get_attr(osic, base, name);
			}
		}
	}

	return value;
}

static struct oobject *
osuper_string(struct osic *osic, struct osuper *self)
{
	char buffer[9];

	snprintf(buffer, sizeof(buffer), "<super>");

	buffer[sizeof(buffer) - 1] = '\0';
	return ostring_create(osic, buffer, strlen(buffer));
}

static struct oobject *
osuper_method(struct osic *osic,
                   struct oobject *self,
                   int method, int argc, struct oobject *argv[])
{
#define cast(a) ((struct osuper *)(a))

	switch (method) {
	case OOBJECT_METHOD_GET_ATTR:
		return osuper_get_attr(osic, cast(self), argv[0]);

	case OOBJECT_METHOD_CALL:
		if (!cast(self)->base) {
			return osuper_call(osic, cast(self), argc, argv);
		}
		return NULL;

	case OOBJECT_METHOD_CALLABLE:
		if (!cast(self)->base) {
			return osic->l_true;
		}
		return osic->l_false;

	case OOBJECT_METHOD_STRING:
		return osuper_string(osic, cast(self));

	default:
		return oobject_default(osic, self, method, argc, argv);
	}
}

void *
osuper_create(struct osic *osic, struct oobject *binding)
{
	struct osuper *self;

	self = oobject_create(osic, sizeof(*self), osuper_method);
	if (self) {
		self->self = binding;
	}

	return self;
}

struct otype *
osuper_type_create(struct osic *osic)
{
	return otype_create(osic, "super", osuper_method, NULL);
}
