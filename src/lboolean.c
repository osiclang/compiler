#include "osic.h"
#include "lstring.h"
#include "lboolean.h"

static struct lobject *
lboolean_eq(struct osic *osic, struct lobject *a, struct lobject *b)
{
	if (a == b) {
		return osic->l_true;
	}
	return osic->l_false;
}

static struct lobject *
lboolean_string(struct osic *osic, struct lobject *boolean)
{
	if (boolean == osic->l_true) {
		return lstring_create(osic, "true", 4);
	}

	return lstring_create(osic, "false", 5);
}

static struct lobject *
lboolean_method(struct osic *osic,
                struct lobject *self,
                int method, int argc, struct lobject *argv[])
{
	switch (method) {
	case LOBJECT_METHOD_EQ:
		return lboolean_eq(osic, self, argv[0]);

	case LOBJECT_METHOD_STRING:
		return lboolean_string(osic, self);

	default:
		return lobject_default(osic, self, method, argc, argv);
	}
}

void *
lboolean_create(struct osic *osic, int value)
{
	struct lboolean *self;

	self = lobject_create(osic, sizeof(*self), lboolean_method);

	return self;
}

static struct lobject *
lboolean_type_method(struct osic *osic,
                     struct lobject *self,
                     int method, int argc, struct lobject *argv[])
{
	switch (method) {
	case LOBJECT_METHOD_CALL:
		if (argv) {
			return lobject_boolean(osic, argv[0]);
		}

	case LOBJECT_METHOD_CALLABLE:
		return osic->l_true;

	default:
		return lobject_default(osic, self, method, argc, argv);
	}
}

struct ltype *
lboolean_type_create(struct osic *osic)
{
	struct ltype *type;

	type = ltype_create(osic,
	                    "boolean",
	                    lboolean_method,
	                    lboolean_type_method);
	if (type) {
		osic_add_global(osic, "boolean", type);
	}

	return type;
}
