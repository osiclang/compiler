#include "osic.h"
#include "larray.h"
#include "ltable.h"
#include "lclass.h"
#include "lstring.h"
#include "linteger.h"
#include "linstance.h"
#include "laccessor.h"

#include <stdio.h>
#include <string.h>

static struct lobject *
linstance_call(struct osic *osic,
               struct lobject *self,
               int argc, struct lobject *argv[])
{
	struct lobject *call;

	call = lobject_get_attr(osic, self, osic->l_call_string);
	if (call) {
		if (!lobject_is_exception(osic, call)) {
			return lobject_call(osic, call, argc, argv);
		}

		return call;
	}

	return lobject_error_not_callable(osic, self);
}

static struct lobject *
linstance_get_callable(struct osic *osic,
                       struct lobject *self,
                       int argc, struct lobject *argv[])
{
	struct lobject *call;

	call = lobject_get_attr(osic, self, osic->l_call_string);
	if (call && !lobject_is_exception(osic, call)) {
		return osic->l_true;
	}

	return osic->l_false;
}

static struct lobject *
linstance_get_attr(struct osic *osic,
                   struct linstance *self,
                   struct lobject *name)
{
	int i;
	long length;
	const char *cstr;
	struct lclass *clazz;
	struct lobject *base;
	struct lobject *value;

	cstr = lstring_to_cstr(osic, name);
	if (strcmp(cstr, "__callable__") == 0) {
		return lfunction_create(osic,
		                        name,
		                        (struct lobject *)self,
		                        linstance_get_callable);
	}

	/* search self */
	value = lobject_get_item(osic, self->attr, name);
	if (value) {
		return value;
	}

	/* search class */
	base = (struct lobject *)self->clazz;
	value = lobject_get_item(osic, self->clazz->attr, name);
	if (!value) {
		/* search class->bases */
		length = larray_length(osic, self->clazz->bases);
		for (i = 0; i < length; i++) {
			base = larray_get_item(osic, self->clazz->bases, i);
			if (lobject_is_class(osic, base)) {
				clazz = (struct lclass *)base;
				value = lobject_get_item(osic,
				                         clazz->attr,
				                         name);
			} else {
				value = lobject_get_attr(osic,
				                         self->native,
				                         name);
			}

			if (value) {
				break;
			}
		}
	}

	if (value) {
		if (lobject_is_function(osic, value)) {
			struct lobject *binding;
			if (base && lobject_is_class(osic, base)) {
				/* make a self binding function */
				binding = (struct lobject *) self;
			} else {
				binding = self->native;
			}

			value = lfunction_bind(osic, value, binding);
		}

		return value;
	}

	return NULL;
}

static struct lobject *
linstance_call_get_attr_callback(struct osic *osic,
                                 struct lframe *frame,
                                 struct lobject *retval)
{
	struct lobject *self;
	struct lobject *name;

	if (retval == osic->l_sentinel) {
		self = frame->self;
		name = lframe_get_item(osic, frame, 0);

		return lobject_error_attribute(osic,
		                               "'%@' has no attribute '%@'",
		                               self,
		                               name);
	}

	return retval;
}

static struct lobject *
linstance_call_get_attr(struct osic *osic,
                        struct linstance *self,
                        struct lobject *name)
{
	const char *cstr;
	struct lframe *frame;
	struct lobject *retval;
	struct lobject *function;

	cstr = lstring_to_cstr(osic, name);
	/*
	 * ignore name start with '__', '__xxx__' is internal use
	 * dynamic replace internal method is too complex
	 */
	if (strncmp(cstr, "__", 2) == 0) {
		return NULL;
	}

	function = linstance_get_attr(osic, self, osic->l_get_attr_string);
	if (!function) {
		return NULL;
	}

	frame = osic_machine_push_new_frame(osic,
	                                     (struct lobject *)self,
	                                     NULL,
	                                     linstance_call_get_attr_callback,
	                                     1);
	if (!frame) {
		return NULL;
	}
	lframe_set_item(osic, frame, 0, name);

	retval = lobject_call(osic, function, 1, &name);

	return osic_machine_return_frame(osic, retval);
}

static struct lobject *
linstance_set_attr(struct osic *osic,
                   struct linstance *self,
                   struct lobject *name,
                   struct lobject *value)
{
	return lobject_set_item(osic, self->attr, name, value);
}

static struct lobject *
linstance_call_set_attr(struct osic *osic,
                        struct linstance *self,
                        struct lobject *name,
                        struct lobject *value)
{
	const char *cstr;
	struct lobject *argv[2];
	struct lobject *function;

	cstr = lstring_to_cstr(osic, name);
	if (strncmp(cstr, "__", 2) == 0) {
		return NULL;
	}

	function = linstance_get_attr(osic, self, osic->l_set_attr_string);
	if (!function) {
		return NULL;
	}
	argv[0] = name;
	argv[1] = value;

	return lobject_call(osic, function, 2, argv);
}

static struct lobject *
linstance_del_attr(struct osic *osic,
                   struct linstance *self,
                   struct lobject *name)
{
	return lobject_del_item(osic, self->attr, name);
}

static struct lobject *
linstance_call_del_attr(struct osic *osic,
                        struct linstance *self,
                        struct lobject *name)
{
	const char *cstr;
	struct lobject *function;

	cstr = lstring_to_cstr(osic, name);
	if (strncmp(cstr, "__", 2) == 0) {
		return NULL;
	}

	function = linstance_get_attr(osic, self, osic->l_del_attr_string);
	if (function) {
		return NULL;
	}
	return lobject_call(osic, function, 1, &name);
}

static struct lobject *
linstance_has_attr_callback(struct osic *osic,
                            struct lframe *frame,
                            struct lobject *retval)
{
	if (retval == osic->l_sentinel) {
		return osic->l_false;
	}

	if (lobject_is_error(osic, retval)) {
		return retval;
	}

	if (retval) {
		return osic->l_true;
	}

	return osic->l_false;
}

static struct lobject *
linstance_has_attr(struct osic *osic,
                   struct linstance *self,
                   struct lobject *name)
{
	int i;
	long length;
	struct lframe *frame;
	struct lobject *base;
	struct lobject *value;
	struct lobject *function;

	/* search self */
	if (lobject_get_item(osic, self->attr, name)) {
		return osic->l_true;
	}

	/* search class */
	base = (struct lobject *)self->clazz;
	if (lobject_get_attr(osic, base, name)) {
		return osic->l_true;
	}

	/* search class->bases */
	length = larray_length(osic, self->clazz->bases);
	for (i = 0; i < length; i++) {
		base = larray_get_item(osic, self->clazz->bases, i);
		if (lobject_is_class(osic, base)) {
			value = lobject_get_attr(osic, base, name);
		} else {
			value = lobject_get_attr(osic,
			                         self->native,
			                         name);
		}

		if (value) {
			return osic->l_true;
		}
	}

	if (self->native &&
	    lobject_has_attr(osic, self->native, name) == osic->l_true)
	{
		return osic->l_true;
	}

	function = linstance_get_attr(osic, self, osic->l_get_attr_string);
	if (!function) {
		return osic->l_false;
	}

	frame = osic_machine_push_new_frame(osic,
	                                     NULL,
	                                     NULL,
	                                     linstance_has_attr_callback,
	                                     0);
	if (!frame) {
		return NULL;
	}
	lobject_call(osic, function, 1, &name);

	return osic->l_nil; /* make a junk value avoid exception */
}

static struct lobject *
linstance_get_getter(struct osic *osic,
                     struct linstance *self,
                     struct lobject *name)
{
	struct lobject *getter;

	getter = lobject_get_getter(osic,
	                            (struct lobject *)self->clazz,
	                            name);
	if (getter) {
		getter = laccessor_create(osic,
		                          ((struct laccessor *)getter)->count,
		                          ((struct laccessor *)getter)->items);
		((struct laccessor *)getter)->self = (struct lobject *)self;

		return getter;
	}

	return NULL;
}

static struct lobject *
linstance_get_setter(struct osic *osic,
                     struct linstance *self,
                     struct lobject *name)
{
	struct lobject *setter;

	setter = lobject_get_setter(osic,
	                            (struct lobject *)self->clazz,
	                            name);
	if (setter) {
		setter = laccessor_create(osic,
		                          ((struct laccessor *)setter)->count,
		                          ((struct laccessor *)setter)->items);
		((struct laccessor *)setter)->self = (struct lobject *)self;

		return setter;
	}

	return NULL;
}

static struct lobject *
linstance_set_item(struct osic *osic,
                   struct linstance *self,
                   struct lobject *name,
                   struct lobject *value)
{
	struct lobject *argv[2];
	struct lobject *function;

	argv[0] = name;
	argv[1] = value;
	function = linstance_get_attr(osic, self, osic->l_set_item_string);
	if (function) {
		return lobject_call(osic, function, 2, argv);
	}

	if (self->native) {
		return lobject_set_item(osic, self->native, name, value);
	}

	return lobject_error_item(osic,
	                          "'%@' unsupport set item",
	                          self);
}

static struct lobject *
linstance_get_item(struct osic *osic,
                   struct linstance *self,
                   struct lobject *name)
{
	struct lobject *argv[1];
	struct lobject *function;

	argv[0] = name;
	function = linstance_get_attr(osic, self, osic->l_get_item_string);
	if (function) {
		lobject_call(osic, function, 1, argv);

		return osic->l_nil; /* make a junk value avoid exception */
	}

	if (self->native) {
		return lobject_get_item(osic, self->native, name);
	}

	return NULL;
}

static struct lobject *
linstance_string(struct osic *osic, struct linstance *self)
{
	char buffer[256];
	if (self->native) {
		return lobject_string(osic, self->native);
	}

	snprintf(buffer,
	         sizeof(buffer),
	         "<instance of %s %p>",
	         lstring_to_cstr(osic, self->clazz->name),
	         (void *)self);
	buffer[sizeof(buffer) - 1] = '\0';

	return lstring_create(osic, buffer, strlen(buffer));
}

static struct lobject *
linstance_mark(struct osic *osic, struct linstance *self)
{
	lobject_mark(osic, self->attr);
	lobject_mark(osic, (struct lobject *)self->clazz);

	if (self->native) {
		lobject_mark(osic, self->native);
	}

	return NULL;
}

static struct lobject *
linstance_method(struct osic *osic,
                 struct lobject *self,
                 int method, int argc, struct lobject *argv[])
{
	struct lobject *attr;

#define cast(a) ((struct linstance *)(a))

	switch (method) {
	case LOBJECT_METHOD_ADD:
		attr = linstance_get_attr(osic,
		                          cast(self),
		                          osic->l_add_string);
		if (attr) {
			return lobject_call(osic, attr, argc, argv);
		}
		return lobject_default(osic, self, method, argc, argv);

	case LOBJECT_METHOD_SUB:
		attr = linstance_get_attr(osic,
		                          cast(self),
		                          osic->l_sub_string);
		if (attr) {
			return lobject_call(osic, attr, argc, argv);
		}
		return lobject_default(osic, self, method, argc, argv);

	case LOBJECT_METHOD_MUL:
		attr = linstance_get_attr(osic,
		                          cast(self),
		                          osic->l_mul_string);
		if (attr) {
			return lobject_call(osic, attr, argc, argv);
		}
		return lobject_default(osic, self, method, argc, argv);

	case LOBJECT_METHOD_DIV:
		attr = linstance_get_attr(osic,
		                          cast(self),
		                          osic->l_div_string);
		if (attr) {
			return lobject_call(osic, attr, argc, argv);
		}
		return lobject_default(osic, self, method, argc, argv);

	case LOBJECT_METHOD_MOD:
		attr = linstance_get_attr(osic,
		                          cast(self),
		                          osic->l_mod_string);
		if (attr) {
			return lobject_call(osic, attr, argc, argv);
		}
		return lobject_default(osic, self, method, argc, argv);

	case LOBJECT_METHOD_SET_ITEM:
		return linstance_set_item(osic, cast(self), argv[0], argv[1]);

	case LOBJECT_METHOD_GET_ITEM:
		return linstance_get_item(osic, cast(self), argv[0]);

	case LOBJECT_METHOD_GET_ATTR: {
		struct lobject *value;

		value = linstance_get_attr(osic, cast(self), argv[0]);
		if (value) {
			return value;
		}
		return linstance_call_get_attr(osic, cast(self), argv[0]);
	}

	case LOBJECT_METHOD_SET_ATTR: {
		struct lobject *value;

		value = linstance_set_attr(osic, cast(self), argv[0], argv[1]);
		if (value) {
			return value;
		}
		return linstance_call_set_attr(osic,
		                               cast(self),
		                               argv[0],
		                               argv[1]);
	}

	case LOBJECT_METHOD_DEL_ATTR: {
		struct lobject *value;

		value = linstance_del_attr(osic, cast(self), argv[0]);
		if (value) {
			return value;
		}
		return linstance_call_del_attr(osic, cast(self), argv[0]);
	}

	case LOBJECT_METHOD_HAS_ATTR:
		return linstance_has_attr(osic, cast(self), argv[0]);

	case LOBJECT_METHOD_GET_GETTER:
		return linstance_get_getter(osic, cast(self), argv[0]);

	case LOBJECT_METHOD_GET_SETTER:
		return linstance_get_setter(osic, cast(self), argv[0]);

	case LOBJECT_METHOD_STRING:
		return linstance_string(osic, cast(self));

	case LOBJECT_METHOD_CALL:
		return linstance_call(osic, self, argc, argv);

	case LOBJECT_METHOD_MARK:
		return linstance_mark(osic, cast(self));

	default:
		return lobject_default(osic, self, method, argc, argv);
	}
}

void *
linstance_create(struct osic *osic, struct lclass *clazz)
{
	struct linstance *self;

	self = lobject_create(osic, sizeof(*self), linstance_method);
	if (self) {
		self->attr = ltable_create(osic);
		self->clazz = clazz;
	}

	return self;
}

struct ltype *
linstance_type_create(struct osic *osic)
{
	return ltype_create(osic, "instance", linstance_method, NULL);
}
