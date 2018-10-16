#include "osic.h"
#include "oVarg.h"

static struct oobject *
ovarg_eq(struct osic *osic, struct ovarg *a, struct ovarg *b)
{
	return oobject_eq(osic, a->arguments, b->arguments);
}

static struct oobject *
ovarg_mark(struct osic *osic, struct ovarg *self)
{
	oobject_mark(osic, self->arguments);

	return NULL;
}

static struct oobject *
ovarg_string(struct osic *osic, struct ovarg *self)
{
	return oobject_string(osic, self->arguments);
}

static struct oobject *
ovarg_method(struct osic *osic,
             struct oobject *self,
             int method, int argc, struct oobject *argv[])
{
#define cast(a) ((struct ovarg *)(a))

	switch (method) {
	case OOBJECT_METHOD_EQ:
		return ovarg_eq(osic, cast(self), cast(argv[0]));

	case OOBJECT_METHOD_MARK:
		return ovarg_mark(osic, cast(self));

	case OOBJECT_METHOD_STRING:
		return ovarg_string(osic, cast(self));

	default:
		return oobject_default(osic, self, method, argc, argv);
	}
}

void *
ovarg_create(struct osic *osic, struct oobject *arguments)
{
	struct ovarg *self;

	self = oobject_create(osic, sizeof(*self), ovarg_method);
	if (self) {
		self->arguments = arguments;
	}

	return self;
}

struct otype *
ovarg_type_create(struct osic *osic)
{
	return otype_create(osic, "varg", ovarg_method, NULL);
}
