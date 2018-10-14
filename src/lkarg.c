#include "osic.h"
#include "lkarg.h"
#include "lstring.h"

#include <stdio.h>
#include <string.h>

static struct lobject *
lkarg_eq(struct osic *osic, struct lkarg *a, struct lkarg *b)
{
	if (lobject_is_equal(osic, a->keyword, b->keyword) &&
	    lobject_is_equal(osic, a->keyword, b->keyword))
	{
		return osic->l_true;
	}
	return osic->l_false;
}

static struct lobject *
lkarg_string(struct osic *osic, struct lkarg *self)
{
	char *buffer;
	unsigned long length;

	struct lobject *keyword;
	struct lobject *argument;

	struct lobject *string;

	keyword = lobject_string(osic, self->keyword);
	argument = lobject_string(osic, self->argument);

	length = 4; /* minimal length '(, )' */
	length += lstring_length(osic, keyword);
	length += lstring_length(osic, argument);

	buffer = osic_allocator_alloc(osic, length);
	if (!buffer) {
		return NULL;
	}
	snprintf(buffer,
	         length,
	         "(%s, %s)",
	         lstring_to_cstr(osic, keyword),
	         lstring_to_cstr(osic, argument));

	string = lstring_create(osic, buffer, length);
	osic_allocator_free(osic, buffer);

	return string;
}

static struct lobject *
lkarg_mark(struct osic *osic, struct lkarg *self)
{
	lobject_mark(osic, self->keyword);
	lobject_mark(osic, self->argument);

	return NULL;
}

static struct lobject *
lkarg_method(struct osic *osic,
             struct lobject *self,
             int method, int argc, struct lobject *argv[])
{
#define cast(a) ((struct lkarg *)(a))

	switch (method) {
	case LOBJECT_METHOD_EQ:
		return lkarg_eq(osic, cast(self), cast(argv[0]));

	case LOBJECT_METHOD_MARK:
		return lkarg_mark(osic, cast(self));

	case LOBJECT_METHOD_STRING:
		return lkarg_string(osic, cast(self));

	default:
		return lobject_default(osic, self, method, argc, argv);
	}
}

void *
lkarg_create(struct osic *osic,
             struct lobject *keyword,
             struct lobject *argument)
{
	struct lkarg *self;

	self = lobject_create(osic, sizeof(*self), lkarg_method);
	if (self) {
		self->keyword = keyword;
		self->argument = argument;
	}

	return self;
}

struct ltype *
lkarg_type_create(struct osic *osic)
{
	return ltype_create(osic, "karg", lkarg_method, NULL);
}
