#include "osic.h"
#include "table.h"
#include "oString.h"

#include <stdio.h>
#include <string.h>

static struct oobject *
otype_string(struct osic *osic, struct otype *self)
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

	return ostring_create(osic, buffer, strlen(buffer));
}

static struct oobject *
otype_method(struct osic *osic,
             struct oobject *self,
             int method, int argc, struct oobject *argv[])
{
#define cast(a) ((struct otype *)(a))

	struct otype *type;

	type = (struct otype *)self;
	switch (method) {
	case OOBJECT_METHOD_EQ:
		if (!oobject_is_type(osic, argv[0])) {
			return osic->l_false;
		}
		if (type->method == cast(argv[0])->method) {
			return osic->l_true;
		}
		return osic->l_false;

	case OOBJECT_METHOD_CALL: {
		struct oframe *frame;

		if (!type->type_method) {
			return oobject_error_not_callable(osic, self);
		}
		frame = osic_machine_push_new_frame(osic,
		                                     NULL,
		                                     self,
		                                     oframe_default_callback,
		                                     0);
		if (!frame) {
			return NULL;
		}
		break;
	}

	case OOBJECT_METHOD_CALLABLE:
		if (!type->type_method) {
			return osic->l_false;
		}
		break;

	case OOBJECT_METHOD_STRING:
		return otype_string(osic, type);

	case OOBJECT_METHOD_DESTROY:
		osic_del_type(osic, type);
		break;

	default:
		break;
	}

	if (!type->type_method) {
		return oobject_default(osic, self, method, argc, argv);
	}
	return type->type_method(osic, self, method, argc, argv);
}

void *
otype_create(struct osic *osic,
             const char *name,
             oobject_method_t method,
             oobject_method_t type_method)
{
	struct otype *self;

	self = oobject_create(osic, sizeof(*self), otype_method);
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

static struct oobject *
otype_type_method(struct osic *osic,
                  struct oobject *self,
                  int method, int argc, struct oobject *argv[])
{
	switch (method) {
	case OOBJECT_METHOD_CALL: {
		if (!argc) {
			return osic->l_nil;
		}

		if (oobject_is_integer(osic, argv[0])) {
			return (struct oobject *)osic->l_integer_type;
		}

		return osic_get_type(osic, argv[0]->l_method);
	}

	case OOBJECT_METHOD_CALLABLE:
		return osic->l_true;

	default:
		return oobject_default(osic, self, method, argc, argv);
	}
}

struct otype *
otype_type_create(struct osic *osic)
{
	struct otype *type;

	type = otype_create(osic, "type", otype_method, otype_type_method);
	if (type) {
		osic_add_global(osic, "type", type);
	}

	return type;
}
