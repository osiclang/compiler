#include "osic.h"
#include "lvkarg.h"

static struct lobject *
lvkarg_eq(struct osic *osic, struct lvkarg *a, struct lvkarg *b)
{
	return lobject_eq(osic, a->arguments, b->arguments);
}

static struct lobject *
lvkarg_map_item(struct osic *osic, struct lvkarg *self)
{
	return lobject_map_item(osic, self->arguments);
}

static struct lobject *
lvkarg_mark(struct osic *osic, struct lvkarg *self)
{
	lobject_mark(osic, self->arguments);

	return NULL;
}

static struct lobject *
lvkarg_string(struct osic *osic, struct lvkarg *self)
{
	return lobject_string(osic, self->arguments);
}

static struct lobject *
lvkarg_method(struct osic *osic,
              struct lobject *self,
              int method, int argc, struct lobject *argv[])
{
#define cast(a) ((struct lvkarg *)(a))

	switch (method) {
	case LOBJECT_METHOD_EQ:
		return lvkarg_eq(osic, cast(self), cast(argv[0]));

	case LOBJECT_METHOD_MAP_ITEM:
		return lvkarg_map_item(osic, cast(self));

	case LOBJECT_METHOD_MARK:
		return lvkarg_mark(osic, cast(self));

	case LOBJECT_METHOD_STRING:
		return lvkarg_string(osic, cast(self));

	default:
		return lobject_default(osic, self, method, argc, argv);
	}
}

void *
lvkarg_create(struct osic *osic, struct lobject *arguments)
{
	struct lvkarg *self;

	self = lobject_create(osic, sizeof(*self), lvkarg_method);
	if (self) {
		self->arguments = arguments;
	}

	return self;
}

struct ltype *
lvkarg_type_create(struct osic *osic)
{
	return ltype_create(osic, "vkarg", lvkarg_method, NULL);
}
