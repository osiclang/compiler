#include "osic.h"
#include "oArray.h"
#include "oTable.h"
#include "oClass.h"
#include "oString.h"
#include "oInteger.h"
#include "oInstance.h"
#include "oAccessor.h"

#include <stdio.h>
#include <string.h>

static struct oobject *
oinstance_call(struct osic *osic,
               struct oobject *self,
               int argc, struct oobject *argv[])
{
	struct oobject *call;

	call = oobject_get_attr(osic, self, osic->l_call_string);
	if (call) {
		if (!oobject_is_exception(osic, call)) {
			return oobject_call(osic, call, argc, argv);
		}

		return call;
	}

	return oobject_error_not_callable(osic, self);
}

static struct oobject *
oinstance_get_callable(struct osic *osic,
                       struct oobject *self,
                       int argc, struct oobject *argv[])
{
	struct oobject *call;

	call = oobject_get_attr(osic, self, osic->l_call_string);
	if (call && !oobject_is_exception(osic, call)) {
		return osic->l_true;
	}

	return osic->l_false;
}

static struct oobject *
oinstance_get_attr(struct osic *osic,
                   struct oinstance *self,
                   struct oobject *name)
{
	int i;
	long length;
	const char *cstr;
	struct oclass *clazz;
	struct oobject *base;
	struct oobject *value;

	cstr = ostring_to_cstr(osic, name);
	if (strcmp(cstr, "__callable__") == 0) {
		return ofunction_create(osic,
		                        name,
		                        (struct oobject *)self,
		                        oinstance_get_callable);
	}

	/* search self */
	value = oobject_get_item(osic, self->attr, name);
	if (value) {
		return value;
	}

	/* search class */
	base = (struct oobject *)self->clazz;
	value = oobject_get_item(osic, self->clazz->attr, name);
	if (!value) {
		/* search class->bases */
		length = oarray_length(osic, self->clazz->bases);
		for (i = 0; i < length; i++) {
			base = oarray_get_item(osic, self->clazz->bases, i);
			if (oobject_is_class(osic, base)) {
				clazz = (struct oclass *)base;
				value = oobject_get_item(osic,
				                         clazz->attr,
				                         name);
			} else {
				value = oobject_get_attr(osic,
				                         self->native,
				                         name);
			}

			if (value) {
				break;
			}
		}
	}

	if (value) {
		if (oobject_is_function(osic, value)) {
			struct oobject *binding;
			if (base && oobject_is_class(osic, base)) {
				/* make a self binding function */
				binding = (struct oobject *) self;
			} else {
				binding = self->native;
			}

			value = ofunction_bind(osic, value, binding);
		}

		return value;
	}

	return NULL;
}

static struct oobject *
oinstance_call_get_attr_callback(struct osic *osic,
                                 struct oframe *frame,
                                 struct oobject *retval)
{
	struct oobject *self;
	struct oobject *name;

	if (retval == osic->l_sentinel) {
		self = frame->self;
		name = oframe_get_item(osic, frame, 0);

		return oobject_error_attribute(osic,
		                               "'%@' has no attribute '%@'",
		                               self,
		                               name);
	}

	return retval;
}

static struct oobject *
oinstance_call_get_attr(struct osic *osic,
                        struct oinstance *self,
                        struct oobject *name)
{
	const char *cstr;
	struct oframe *frame;
	struct oobject *retval;
	struct oobject *function;

	cstr = ostring_to_cstr(osic, name);
	/*
	 * ignore name start with '__', '__xxx__' is internal use
	 * dynamic replace internal method is too complex
	 */
	if (strncmp(cstr, "__", 2) == 0) {
		return NULL;
	}

	function = oinstance_get_attr(osic, self, osic->l_get_attr_string);
	if (!function) {
		return NULL;
	}

	frame = osic_machine_push_new_frame(osic,
	                                     (struct oobject *)self,
	                                     NULL,
	                                     oinstance_call_get_attr_callback,
	                                     1);
	if (!frame) {
		return NULL;
	}
	oframe_set_item(osic, frame, 0, name);

	retval = oobject_call(osic, function, 1, &name);

	return osic_machine_return_frame(osic, retval);
}

static struct oobject *
oinstance_set_attr(struct osic *osic,
                   struct oinstance *self,
                   struct oobject *name,
                   struct oobject *value)
{
	return oobject_set_item(osic, self->attr, name, value);
}

static struct oobject *
oinstance_call_set_attr(struct osic *osic,
                        struct oinstance *self,
                        struct oobject *name,
                        struct oobject *value)
{
	const char *cstr;
	struct oobject *argv[2];
	struct oobject *function;

	cstr = ostring_to_cstr(osic, name);
	if (strncmp(cstr, "__", 2) == 0) {
		return NULL;
	}

	function = oinstance_get_attr(osic, self, osic->l_set_attr_string);
	if (!function) {
		return NULL;
	}
	argv[0] = name;
	argv[1] = value;

	return oobject_call(osic, function, 2, argv);
}

static struct oobject *
oinstance_del_attr(struct osic *osic,
                   struct oinstance *self,
                   struct oobject *name)
{
	return oobject_del_item(osic, self->attr, name);
}

static struct oobject *
oinstance_call_del_attr(struct osic *osic,
                        struct oinstance *self,
                        struct oobject *name)
{
	const char *cstr;
	struct oobject *function;

	cstr = ostring_to_cstr(osic, name);
	if (strncmp(cstr, "__", 2) == 0) {
		return NULL;
	}

	function = oinstance_get_attr(osic, self, osic->l_del_attr_string);
	if (function) {
		return NULL;
	}
	return oobject_call(osic, function, 1, &name);
}

static struct oobject *
oinstance_has_attr_callback(struct osic *osic,
                            struct oframe *frame,
                            struct oobject *retval)
{
	if (retval == osic->l_sentinel) {
		return osic->l_false;
	}

	if (oobject_is_error(osic, retval)) {
		return retval;
	}

	if (retval) {
		return osic->l_true;
	}

	return osic->l_false;
}

static struct oobject *
oinstance_has_attr(struct osic *osic,
                   struct oinstance *self,
                   struct oobject *name)
{
	int i;
	long length;
	struct oframe *frame;
	struct oobject *base;
	struct oobject *value;
	struct oobject *function;

	/* search self */
	if (oobject_get_item(osic, self->attr, name)) {
		return osic->l_true;
	}

	/* search class */
	base = (struct oobject *)self->clazz;
	if (oobject_get_attr(osic, base, name)) {
		return osic->l_true;
	}

	/* search class->bases */
	length = oarray_length(osic, self->clazz->bases);
	for (i = 0; i < length; i++) {
		base = oarray_get_item(osic, self->clazz->bases, i);
		if (oobject_is_class(osic, base)) {
			value = oobject_get_attr(osic, base, name);
		} else {
			value = oobject_get_attr(osic,
			                         self->native,
			                         name);
		}

		if (value) {
			return osic->l_true;
		}
	}

	if (self->native &&
	    oobject_has_attr(osic, self->native, name) == osic->l_true)
	{
		return osic->l_true;
	}

	function = oinstance_get_attr(osic, self, osic->l_get_attr_string);
	if (!function) {
		return osic->l_false;
	}

	frame = osic_machine_push_new_frame(osic,
	                                     NULL,
	                                     NULL,
	                                     oinstance_has_attr_callback,
	                                     0);
	if (!frame) {
		return NULL;
	}
	oobject_call(osic, function, 1, &name);

	return osic->l_nil; /* make a junk value avoid exception */
}

static struct oobject *
oinstance_get_getter(struct osic *osic,
                     struct oinstance *self,
                     struct oobject *name)
{
	struct oobject *getter;

	getter = oobject_get_getter(osic,
	                            (struct oobject *)self->clazz,
	                            name);
	if (getter) {
		getter = oaccessor_create(osic,
		                          ((struct oaccessor *)getter)->count,
		                          ((struct oaccessor *)getter)->items);
		((struct oaccessor *)getter)->self = (struct oobject *)self;

		return getter;
	}

	return NULL;
}

static struct oobject *
oinstance_get_setter(struct osic *osic,
                     struct oinstance *self,
                     struct oobject *name)
{
	struct oobject *setter;

	setter = oobject_get_setter(osic,
	                            (struct oobject *)self->clazz,
	                            name);
	if (setter) {
		setter = oaccessor_create(osic,
		                          ((struct oaccessor *)setter)->count,
		                          ((struct oaccessor *)setter)->items);
		((struct oaccessor *)setter)->self = (struct oobject *)self;

		return setter;
	}

	return NULL;
}

static struct oobject *
oinstance_set_item(struct osic *osic,
                   struct oinstance *self,
                   struct oobject *name,
                   struct oobject *value)
{
	struct oobject *argv[2];
	struct oobject *function;

	argv[0] = name;
	argv[1] = value;
	function = oinstance_get_attr(osic, self, osic->l_set_item_string);
	if (function) {
		return oobject_call(osic, function, 2, argv);
	}

	if (self->native) {
		return oobject_set_item(osic, self->native, name, value);
	}

	return oobject_error_item(osic,
	                          "'%@' unsupport set item",
	                          self);
}

static struct oobject *
oinstance_get_item(struct osic *osic,
                   struct oinstance *self,
                   struct oobject *name)
{
	struct oobject *argv[1];
	struct oobject *function;

	argv[0] = name;
	function = oinstance_get_attr(osic, self, osic->l_get_item_string);
	if (function) {
		oobject_call(osic, function, 1, argv);

		return osic->l_nil; /* make a junk value avoid exception */
	}

	if (self->native) {
		return oobject_get_item(osic, self->native, name);
	}

	return NULL;
}

static struct oobject *
oinstance_string(struct osic *osic, struct oinstance *self)
{
	char buffer[256];
	if (self->native) {
		return oobject_string(osic, self->native);
	}

	snprintf(buffer,
	         sizeof(buffer),
	         "<instance of %s %p>",
	         ostring_to_cstr(osic, self->clazz->name),
	         (void *)self);
	buffer[sizeof(buffer) - 1] = '\0';

	return ostring_create(osic, buffer, strlen(buffer));
}

static struct oobject *
oinstance_mark(struct osic *osic, struct oinstance *self)
{
	oobject_mark(osic, self->attr);
	oobject_mark(osic, (struct oobject *)self->clazz);

	if (self->native) {
		oobject_mark(osic, self->native);
	}

	return NULL;
}

static struct oobject *
oinstance_method(struct osic *osic,
                 struct oobject *self,
                 int method, int argc, struct oobject *argv[])
{
	struct oobject *attr;

#define cast(a) ((struct oinstance *)(a))

	switch (method) {
	case OOBJECT_METHOD_ADD:
		attr = oinstance_get_attr(osic,
		                          cast(self),
		                          osic->l_add_string);
		if (attr) {
			return oobject_call(osic, attr, argc, argv);
		}
		return oobject_default(osic, self, method, argc, argv);

	case OOBJECT_METHOD_SUB:
		attr = oinstance_get_attr(osic,
		                          cast(self),
		                          osic->l_sub_string);
		if (attr) {
			return oobject_call(osic, attr, argc, argv);
		}
		return oobject_default(osic, self, method, argc, argv);

	case OOBJECT_METHOD_MUL:
		attr = oinstance_get_attr(osic,
		                          cast(self),
		                          osic->l_mul_string);
		if (attr) {
			return oobject_call(osic, attr, argc, argv);
		}
		return oobject_default(osic, self, method, argc, argv);

	case OOBJECT_METHOD_DIV:
		attr = oinstance_get_attr(osic,
		                          cast(self),
		                          osic->l_div_string);
		if (attr) {
			return oobject_call(osic, attr, argc, argv);
		}
		return oobject_default(osic, self, method, argc, argv);

	case OOBJECT_METHOD_MOD:
		attr = oinstance_get_attr(osic,
		                          cast(self),
		                          osic->l_mod_string);
		if (attr) {
			return oobject_call(osic, attr, argc, argv);
		}
		return oobject_default(osic, self, method, argc, argv);

	case OOBJECT_METHOD_SET_ITEM:
		return oinstance_set_item(osic, cast(self), argv[0], argv[1]);

	case OOBJECT_METHOD_GET_ITEM:
		return oinstance_get_item(osic, cast(self), argv[0]);

	case OOBJECT_METHOD_GET_ATTR: {
		struct oobject *value;

		value = oinstance_get_attr(osic, cast(self), argv[0]);
		if (value) {
			return value;
		}
		return oinstance_call_get_attr(osic, cast(self), argv[0]);
	}

	case OOBJECT_METHOD_SET_ATTR: {
		struct oobject *value;

		value = oinstance_set_attr(osic, cast(self), argv[0], argv[1]);
		if (value) {
			return value;
		}
		return oinstance_call_set_attr(osic,
		                               cast(self),
		                               argv[0],
		                               argv[1]);
	}

	case OOBJECT_METHOD_DEL_ATTR: {
		struct oobject *value;

		value = oinstance_del_attr(osic, cast(self), argv[0]);
		if (value) {
			return value;
		}
		return oinstance_call_del_attr(osic, cast(self), argv[0]);
	}

	case OOBJECT_METHOD_HAS_ATTR:
		return oinstance_has_attr(osic, cast(self), argv[0]);

	case OOBJECT_METHOD_GET_GETTER:
		return oinstance_get_getter(osic, cast(self), argv[0]);

	case OOBJECT_METHOD_GET_SETTER:
		return oinstance_get_setter(osic, cast(self), argv[0]);

	case OOBJECT_METHOD_STRING:
		return oinstance_string(osic, cast(self));

	case OOBJECT_METHOD_CALL:
		return oinstance_call(osic, self, argc, argv);

	case OOBJECT_METHOD_MARK:
		return oinstance_mark(osic, cast(self));

	default:
		return oobject_default(osic, self, method, argc, argv);
	}
}

void *
oinstance_create(struct osic *osic, struct oclass *clazz)
{
	struct oinstance *self;

	self = oobject_create(osic, sizeof(*self), oinstance_method);
	if (self) {
		self->attr = otable_create(osic);
		self->clazz = clazz;
	}

	return self;
}

struct otype *
oinstance_type_create(struct osic *osic)
{
	return otype_create(osic, "instance", oinstance_method, NULL);
}
