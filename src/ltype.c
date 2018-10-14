#include "osic.h"
#include "table.h"
#include "lstring.h"

#include <stdio.h>
#include <string.h>

static struct lobject *
ltype_string(struct osic *osic, struct ltype *self)
{
	char buffer[64];

	if (self->name) {
		snprintf(buffer, sizeof(buffer), "<type %s>", self->name);
	} else {
		snprintf(buffer,
		         sizeof(buffer),
		         "<type %p>",
		         (void *)(uintptr_t)self->method);
	}
	buffer[sizeof(buffer) - 1] = '\0';

	return lstring_create(osic, buffer, strlen(buffer));
}

static struct lobject *
ltype_method(struct osic *osic,
             struct lobject *self,
             int method, int argc, struct lobject *argv[])
{
#define cast(a) ((struct ltype *)(a))

	struct ltype *type;

	type = (struct ltype *)self;
	switch (method) {
	case LOBJECT_METHOD_EQ:
		if (!lobject_is_type(osic, argv[0])) {
			return osic->l_false;
		}
		if (type->method == cast(argv[0])->method) {
			return osic->l_true;
		}
		return osic->l_false;

	case LOBJECT_METHOD_CALL: {
		struct lframe *frame;

		if (!type->type_method) {
			return lobject_error_not_callable(osic, self);
		}
		frame = osic_machine_push_new_frame(osic,
		                                     NULL,
		                                     self,
		                                     lframe_default_callback,
		                                     0);
		if (!frame) {
			return NULL;
		}
		break;
	}

	case LOBJECT_METHOD_CALLABLE:
		if (!type->type_method) {
			return osic->l_false;
		}
		break;

	case LOBJECT_METHOD_STRING:
		return ltype_string(osic, type);

	case LOBJECT_METHOD_DESTROY:
		osic_del_type(osic, type);
		break;

	default:
		break;
	}

	if (!type->type_method) {
		return lobject_default(osic, self, method, argc, argv);
	}
	return type->type_method(osic, self, method, argc, argv);
}

void *
ltype_create(struct osic *osic,
             const char *name,
             lobject_method_t method,
             lobject_method_t type_method)
{
	struct ltype *self;

	self = lobject_create(osic, sizeof(*self), ltype_method);
	if (self) {
		self->name = name;
		self->method = method;
		self->type_method = type_method;
		if (!osic_add_type(osic, self)) {
			return NULL;
		}
	}

	return self;
}

static struct lobject *
ltype_type_method(struct osic *osic,
                  struct lobject *self,
                  int method, int argc, struct lobject *argv[])
{
	switch (method) {
	case LOBJECT_METHOD_CALL: {
		if (!argc) {
			return osic->l_nil;
		}

		if (lobject_is_integer(osic, argv[0])) {
			return (struct lobject *)osic->l_integer_type;
		}

		return osic_get_type(osic, argv[0]->l_method);
	}

	case LOBJECT_METHOD_CALLABLE:
		return osic->l_true;

	default:
		return lobject_default(osic, self, method, argc, argv);
	}
}

struct ltype *
ltype_type_create(struct osic *osic)
{
	struct ltype *type;

	type = ltype_create(osic, "type", ltype_method, ltype_type_method);
	if (type) {
		osic_add_global(osic, "type", type);
	}

	return type;
}
