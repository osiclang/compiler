#include "osic.h"
#include "oTable.h"
#include "oModule.h"
#include "oString.h"
#include "oInteger.h"
#include "oAccessor.h"

#include <stdio.h>
#include <string.h>

static struct oobject *
omodule_get_attr(struct osic *osic,
                 struct omodule *self,
                 struct oobject *name)
{
	int i;
	struct oobject *local;

	local = oobject_get_item(osic, self->attr, name);
	if (!local) {
		return NULL;
	}

	if (self->frame) {
		i = (int)ointeger_to_long(osic, local);

		return oframe_get_item(osic, self->frame, i);
	}

	return local;
}

static struct oobject *
omodule_set_attr(struct osic *osic,
                 struct omodule *self,
                 struct oobject *name,
                 struct oobject *value)
{
	int i;
	struct oobject *local;

	if (self->frame) {
		local = oobject_get_item(osic, self->attr, name);
		if (!local) {
			const char *fmt;

			fmt = "'%@' has no attribute '%@'";
			return oobject_error_attribute(osic, fmt, self, name);
		}

		i = (int)ointeger_to_long(osic, local);
		return oframe_set_item(osic, self->frame, i, value);
	}

	return oobject_set_item(osic, self->attr, name, value);
}

static struct oobject *
omodule_del_attr(struct osic *osic,
                 struct omodule *self,
                 struct oobject *name)
{
	return oobject_del_item(osic, self->attr, name);
}

static struct oobject *
omodule_get_setter(struct osic *osic,
                   struct omodule *self,
                   struct oobject *name)
{
	return oobject_get_item(osic, self->setter, name);
}

static struct oobject *
omodule_set_setter(struct osic *osic,
                   struct omodule *self,
                   int argc, struct oobject *argv[])
{
	struct oobject *setter;

	setter = oaccessor_create(osic, argc - 1, &argv[1]);
	if (!setter) {
		return NULL;
	}

	return oobject_set_item(osic, self->setter, argv[0], setter);
}

static struct oobject *
omodule_get_getter(struct osic *osic,
                   struct omodule *self,
                   struct oobject *name)
{
	return oobject_get_item(osic, self->getter, name);
}

static struct oobject *
omodule_set_getter(struct osic *osic,
                   struct omodule *self,
                   int argc, struct oobject *argv[])
{
	struct oobject *getter;

	getter = oaccessor_create(osic, argc - 1, &argv[1]);
	if (!getter) {
		return NULL;
	}

	return oobject_set_item(osic, self->getter, argv[0], getter);
}

static struct oobject *
omodule_mark(struct osic *osic, struct omodule *self)
{
	oobject_mark(osic, self->name);
	oobject_mark(osic, self->attr);
	oobject_mark(osic, self->getter);
	oobject_mark(osic, self->setter);

	if (self->frame) {
		oobject_mark(osic, (struct oobject *)self->frame);
	}

	return NULL;
}

static struct oobject *
omodule_string(struct osic *osic, struct omodule *self)
{
	char buffer[256];

	snprintf(buffer,
	         sizeof(buffer),
	         "<module '%s'>",
	         ostring_to_cstr(osic, self->name));
	buffer[sizeof(buffer) - 1] = '\0';

	return ostring_create(osic, buffer, strlen(buffer));
}

static struct oobject *
omodule_method(struct osic *osic,
               struct oobject *self,
               int method, int argc, struct oobject *argv[])
{
#define cast(a) ((struct omodule *)(a))

	switch (method) {
	case OOBJECT_METHOD_GET_ATTR:
		return omodule_get_attr(osic, cast(self), argv[0]);

	case OOBJECT_METHOD_SET_ATTR:
		return omodule_set_attr(osic, cast(self), argv[0], argv[1]);

	case OOBJECT_METHOD_DEL_ATTR:
		return omodule_del_attr(osic, cast(self), argv[0]);

	case OOBJECT_METHOD_GET_SETTER:
		return omodule_get_setter(osic, cast(self), argv[0]);

	case OOBJECT_METHOD_SET_SETTER:
		return omodule_set_setter(osic, cast(self), argc, argv);

	case OOBJECT_METHOD_GET_GETTER:
		return omodule_get_getter(osic, cast(self), argv[0]);

	case OOBJECT_METHOD_SET_GETTER:
		return omodule_set_getter(osic, cast(self), argc, argv);

	case OOBJECT_METHOD_MARK:
		return omodule_mark(osic, cast(self));

	case OOBJECT_METHOD_STRING:
		return omodule_string(osic, cast(self));

	default:
		return oobject_default(osic, self, method, argc, argv);
	}
}

void *
omodule_create(struct osic *osic, struct oobject *name)
{
	struct omodule *self;

	self = oobject_create(osic, sizeof(*self), omodule_method);
	if (self) {
		self->name = name;
		self->attr = otable_create(osic);
		if (!self->attr) {
			return NULL;
		}
		self->getter = otable_create(osic);
		if (!self->getter) {
			return NULL;
		}
		self->setter = otable_create(osic);
		if (!self->setter) {
			return NULL;
		}
	}

	return self;
}

struct otype *
omodule_type_create(struct osic *osic)
{
	return otype_create(osic, "module", omodule_method, NULL);
}
