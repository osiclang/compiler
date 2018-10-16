#include "osic.h"
#include "oVkarg.h"

static struct oobject *
ovkarg_eq(struct osic *osic, struct ovkarg *a, struct ovkarg *b)
{
	return oobject_eq(osic, a->arguments, b->arguments);
}

static struct oobject *
ovkarg_map_item(struct osic *osic, struct ovkarg *self)
{
	return oobject_map_item(osic, self->arguments);
}

static struct oobject *
ovkarg_mark(struct osic *osic, struct ovkarg *self)
{
	oobject_mark(osic, self->arguments);

	return NULL;
}

static struct oobject *
ovkarg_string(struct osic *osic, struct ovkarg *self)
{
	return oobject_string(osic, self->arguments);
}

static struct oobject *
ovkarg_method(struct osic *osic,
              struct oobject *self,
              int method, int argc, struct oobject *argv[])
{
#define cast(a) ((struct ovkarg *)(a))

	switch (method) {
	case OOBJECT_METHOD_EQ:
		return ovkarg_eq(osic, cast(self), cast(argv[0]));

	case OOBJECT_METHOD_MAP_ITEM:
		return ovkarg_map_item(osic, cast(self));

	case OOBJECT_METHOD_MARK:
		return ovkarg_mark(osic, cast(self));

	case OOBJECT_METHOD_STRING:
		return ovkarg_string(osic, cast(self));

	default:
		return oobject_default(osic, self, method, argc, argv);
	}
}

void *
ovkarg_create(struct osic *osic, struct oobject *arguments)
{
	struct ovkarg *self;

	self = oobject_create(osic, sizeof(*self), ovkarg_method);
	if (self) {
		self->arguments = arguments;
	}

	return self;
}

struct otype *
ovkarg_type_create(struct osic *osic)
{
	return otype_create(osic, "vkarg", ovkarg_method, NULL);
}
