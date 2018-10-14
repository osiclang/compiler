#include "osic.h"
#include "lvarg.h"

static struct lobject *
lvarg_eq(struct osic *osic, struct lvarg *a, struct lvarg *b)
{
	return lobject_eq(osic, a->arguments, b->arguments);
}

static struct lobject *
lvarg_mark(struct osic *osic, struct lvarg *self)
{
	lobject_mark(osic, self->arguments);

	return NULL;
}

static struct lobject *
lvarg_string(struct osic *osic, struct lvarg *self)
{
	return lobject_string(osic, self->arguments);
}

static struct lobject *
lvarg_method(struct osic *osic,
             struct lobject *self,
             int method, int argc, struct lobject *argv[])
{
#define cast(a) ((struct lvarg *)(a))

	switch (method) {
	case LOBJECT_METHOD_EQ:
		return lvarg_eq(osic, cast(self), cast(argv[0]));

	case LOBJECT_METHOD_MARK:
		return lvarg_mark(osic, cast(self));

	case LOBJECT_METHOD_STRING:
		return lvarg_string(osic, cast(self));

	default:
		return lobject_default(osic, self, method, argc, argv);
	}
}

void *
lvarg_create(struct osic *osic, struct lobject *arguments)
{
	struct lvarg *self;

	self = lobject_create(osic, sizeof(*self), lvarg_method);
	if (self) {
		self->arguments = arguments;
	}

	return self;
}

struct ltype *
lvarg_type_create(struct osic *osic)
{
	return ltype_create(osic, "varg", lvarg_method, NULL);
}
