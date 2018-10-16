#include "osic.h"
#include "oString.h"
#include "oBoolean.h"

static struct oobject *
oboolean_eq(struct osic *osic, struct oobject *a, struct oobject *b)
{
	if (a == b) {
		return osic->l_true;
	}
	return osic->l_false;
}

static struct oobject *
oboolean_string(struct osic *osic, struct oobject *boolean)
{
	if (boolean == osic->l_true) {
		return ostring_create(osic, "true", 4);
	}

	return ostring_create(osic, "false", 5);
}

static struct oobject *
oboolean_method(struct osic *osic,
                struct oobject *self,
                int method, int argc, struct oobject *argv[])
{
	switch (method) {
	case OOBJECT_METHOD_EQ:
		return oboolean_eq(osic, self, argv[0]);

	case OOBJECT_METHOD_STRING:
		return oboolean_string(osic, self);

	default:
		return oobject_default(osic, self, method, argc, argv);
	}
}

void *
oboolean_create(struct osic *osic, int value)
{
	struct oboolean *self;

	self = oobject_create(osic, sizeof(*self), oboolean_method);

	return self;
}

static struct oobject *
oboolean_type_method(struct osic *osic,
                     struct oobject *self,
                     int method, int argc, struct oobject *argv[])
{
	switch (method) {
	case OOBJECT_METHOD_CALL:
		if (argv) {
			return oobject_boolean(osic, argv[0]);
		}

	case OOBJECT_METHOD_CALLABLE:
		return osic->l_true;

	default:
		return oobject_default(osic, self, method, argc, argv);
	}
}

struct otype *
oboolean_type_create(struct osic *osic)
{
	struct otype *type;

	type = otype_create(osic,
	                    "boolean",
	                    oboolean_method,
	                    oboolean_type_method);
	if (type) {
		osic_add_global(osic, "boolean", type);
	}

	return type;
}
