#include "osic.h"
#include "oTable.h"
#include "oArray.h"
#include "oClass.h"
#include "oString.h"
#include "oInteger.h"
#include "oInstance.h"
#include "oAccessor.h"

#include <stdio.h>
#include <string.h>

static struct oobject *
oclass_get_attr(struct osic *osic, struct oclass *self, struct oobject *name)
{
	int i;
	long length;
	struct oclass *clazz;
	struct oobject *base;
	struct oobject *value;

	value = oobject_get_item(osic, self->attr, name);
	if (!value) {
		/* search class->bases */
		length = oarray_length(osic, self->bases);
		for (i = 0; i < length; i++) {
			base = oarray_get_item(osic, self->bases, i);
			if (oobject_is_class(osic, base)) {
				clazz = (struct oclass *)base;
				value = oobject_get_item(osic,
				                         clazz->attr,
				                         name);
			} else {
				value = oobject_get_attr(osic,
				                         base,
				                         name);
			}

			if (value) {
				return value;
			}
		}
	}

	return value;
}

static struct oobject *
oclass_set_attr(struct osic *osic,
                struct oclass *self,
                struct oobject *name,
                struct oobject *value)
{
	return oobject_set_item(osic, self->attr, name, value);
}

static struct oobject *
oclass_del_attr(struct osic *osic, struct oclass *self, struct oobject *name)
{
	return oobject_del_item(osic, self->attr, name);
}

static struct oobject *
oclass_get_setter(struct osic *osic,
                  struct oclass *self,
                  struct oobject *name)
{
	return oobject_get_item(osic, self->setter, name);
}

static struct oobject *
oclass_set_setter(struct osic *osic,
                  struct oclass *self,
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
oclass_get_getter(struct osic *osic,
                  struct oclass *self,
                  struct oobject *name)
{
	return oobject_get_item(osic, self->getter, name);
}

static struct oobject *
oclass_set_getter(struct osic *osic,
                  struct oclass *self,
                  int argc, struct oobject *argv[])
{
	struct oobject *getter;

	getter = oaccessor_create(osic, argc - 1, &argv[1]);
	if (!getter) {
		return NULL;
	}

	return oobject_set_item(osic, self->getter, argv[0], getter);
}

int
oclass_check_slot(struct osic *osic,
                  struct oobject *base,
                  int nslots, struct oobject **slots)
{
	int i;
	int j;
	int candidate;
	struct oobject *item;

	candidate = 1;
	for (i = 0; i < nslots; i++) {
		for (j = 1; j < oarray_length(osic, slots[i]); j++) {
			item = oarray_get_item(osic, slots[i], j);
			if (oobject_is_equal(osic, base, item)) {
				candidate = 0;
			}
		}
	}

	return candidate;
}

void
oclass_del_slot(struct osic *osic,
                struct oobject *base,
                int nslots, struct oobject **slots)
{
	int i;
	int j;
	struct oobject *item;

	for (i = 0; i < nslots; i++) {
		for (j = 0; j < oarray_length(osic, slots[i]); j++) {
			item = oarray_get_item(osic, slots[i], j);
			if (oobject_is_equal(osic, base, item)) {
				oarray_del_item(osic, slots[i], j);
				j -= 1;
			}
		}
	}
}

static struct oobject *
oclass_set_supers(struct osic *osic,
                  struct oclass *self,
                  int nsupers, struct oobject *supers[])
{
	int i;
	int finished;
	struct oobject *zuper;
	struct oobject *bases;

	int n;
	int nonclass;
	struct oobject **slots;

	/* prepare merge slot */
	n = nsupers;
	nonclass = 0;
	slots = osic_allocator_alloc(osic,
	                              sizeof(struct oobject *) * nsupers);
	if (!slots) {
		return NULL;
	}
	for (i = 0; i < nsupers; i++) {
		zuper = supers[nsupers - i - 1];
		slots[i] = oarray_create(osic, 1, &zuper);
		if (!slots[i]) {
			return NULL;
		}

		if (oobject_is_class(osic, supers[nsupers - i - 1])) {
			bases = ((struct oclass *)zuper)->bases;
			slots[i] = oobject_binop(osic,
			                         OOBJECT_METHOD_ADD,
			                         slots[i],
			                         bases);
		} else if (nonclass) {
			osic_allocator_free(osic, slots);

			return oobject_error_type(osic,
			                          "'%@' inherit conflict",
			                          self);
		} else {
			nonclass = 1;
		}
	}

	/* algorithm from https://www.python.org/download/releases/2.3/mro/ */
	for (;;) {
		struct oobject *cand;

		/* check finished */
		finished = 1;
		for (i = 0; i < n; i++) {
			if (oarray_length(osic, slots[i]) > 0) {
				finished = 0;
				break;
			}
		}

		if (finished) {
			break;
		}

		/* merge */
		cand = NULL;
		for (i = 0; i < n; i++) {
			if (oarray_length(osic, slots[i])) {
				cand = oarray_get_item(osic, slots[i], 0);
				if (oclass_check_slot(osic, cand, n, slots)) {
					break;
				}

				cand = NULL;
			}
		}

		if (!cand) {
			osic_allocator_free(osic, slots);

			return oobject_error_type(osic,
			                          "'%@' inconsistent hierarchy",
			                          self);
		}

		if (!oarray_append(osic, self->bases, 1, &cand)) {
			return NULL;
		}
		oclass_del_slot(osic, cand, n, slots);
	}
	osic_allocator_free(osic, slots);

	return osic->l_nil;
}

static struct oobject *
oclass_frame_callback(struct osic *osic,
                      struct oframe *frame,
                      struct oobject *retval)
{
	if (oobject_is_error(osic, retval)) {
		return retval;
	}
	return frame->self;
}

static struct oobject *
oclass_call(struct osic *osic,
            struct oobject *self,
            int argc, struct oobject *argv[])
{
	int i;
	struct oframe *frame;
	struct oclass *clazz;
	struct oobject *base;
	struct oobject *native;
	struct oobject *function;
	struct oinstance *instance;

	clazz = (struct oclass *)self;
	instance = oinstance_create(osic, clazz);
	if (!instance) {
		return NULL;
	}

	for (i = 0; i < oarray_length(osic, clazz->bases); i++) {
		base = oarray_get_item(osic, clazz->bases, i);
		if (!oobject_is_class(osic, base)) {
			native = oobject_call(osic, base, argc, argv);
			if (oobject_is_error(osic, native)) {
				return instance->native;
			}
			instance->native = native;
			oobject_method_call(osic,
			                    instance->native,
			                    OOBJECT_METHOD_INSTANCE,
			                    1,
			                    (struct oobject **)&instance);

			break;
		}
	}

	frame = osic_machine_push_new_frame(osic,
	                                     (struct oobject *)instance,
	                                     NULL,
	                                     oclass_frame_callback,
	                                     0);
	if (!frame) {
		return NULL;
	}

	function = oobject_get_attr(osic,
	                            (struct oobject *)instance,
	                            osic->l_init_string);
	if (function && !oobject_is_error(osic, function)) {
		return oobject_call(osic, function, argc, argv);
	}

	return (struct oobject *)instance;
}

static struct oobject *
oclass_mark(struct osic *osic, struct oclass *self)
{
	oobject_mark(osic, self->name);
	oobject_mark(osic, self->bases);
	oobject_mark(osic, self->attr);
	oobject_mark(osic, self->getter);
	oobject_mark(osic, self->setter);

	return NULL;
}

static struct oobject *
oclass_string(struct osic *osic, struct oclass *self)
{
	char buffer[32];

	snprintf(buffer,
	         sizeof(buffer),
	         "<class '%s'>",
	         ostring_to_cstr(osic, self->name));
	buffer[sizeof(buffer) - 1] = '\0';

	return ostring_create(osic, buffer, strlen(buffer));
}

static struct oobject *
oclass_method(struct osic *osic,
              struct oobject *self,
              int method, int argc, struct oobject *argv[])
{
#define cast(a) ((struct oclass *)(a))

	switch (method) {
	case OOBJECT_METHOD_GET_ATTR:
		return oclass_get_attr(osic, cast(self), argv[0]);

	case OOBJECT_METHOD_SET_ATTR:
		return oclass_set_attr(osic, cast(self), argv[0], argv[1]);

	case OOBJECT_METHOD_DEL_ATTR:
		return oclass_del_attr(osic, cast(self), argv[0]);

	case OOBJECT_METHOD_GET_SETTER:
		return oclass_get_setter(osic, cast(self), argv[0]);

	case OOBJECT_METHOD_SET_SETTER:
		return oclass_set_setter(osic, cast(self), argc, argv);

	case OOBJECT_METHOD_GET_GETTER:
		return oclass_get_getter(osic, cast(self), argv[0]);

	case OOBJECT_METHOD_SET_GETTER:
		return oclass_set_getter(osic, cast(self), argc, argv);

	case OOBJECT_METHOD_CALL:
		return oclass_call(osic, self, argc, argv);

	case OOBJECT_METHOD_CALLABLE:
		return osic->l_true;

	case OOBJECT_METHOD_MARK:
		return oclass_mark(osic, cast(self));

	case OOBJECT_METHOD_STRING:
		return oclass_string(osic, cast(self));

	default:
		return oobject_default(osic, self, method, argc, argv);
	}
}

void *
oclass_create(struct osic *osic,
              struct oobject *name,
              int nsupers,
              struct oobject *supers[],
              int nattrs,
              struct oobject *attrs[])
{
	int i;
	struct oclass *self;
	struct oobject *error;

	self = oobject_create(osic, sizeof(*self), oclass_method);
	if (self) {
		self->name = name;
		self->bases = oarray_create(osic, 0, NULL);
		if (!self->bases) {
			return NULL;
		}
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

		error = oclass_set_supers(osic, self, nsupers, supers);
		if (!error || oobject_is_error(osic, error)) {
			return error;
		}

		for (i = 0; i < nattrs; i += 2) {
			if (!oobject_is_string(osic, attrs[i])) {
				return NULL;
			}

			if (!oobject_set_item(osic,
			                      self->attr,
			                      attrs[i],
			                      attrs[i + 1]))
			{
				return NULL;
			}
		}

		for (i = 0; i < oarray_length(osic, self->bases); i++) {
			oobject_method_call(osic,
			                    oarray_get_item(osic,
			                                    self->bases,
			                                    i),
			                    OOBJECT_METHOD_SUBCLASS,
			                    1,
			                    (struct oobject **)&self);
		}
	}

	return self;
}

struct otype *
oclass_type_create(struct osic *osic)
{
	return otype_create(osic, "class", oclass_method, NULL);
}
