#include "osic.h"
#include "oKarg.h"
#include "oString.h"

#include <stdio.h>
#include <string.h>

static struct oobject *
okarg_eq(struct osic *osic, struct okarg *a, struct okarg *b)
{
	if (oobject_is_equal(osic, a->keyword, b->keyword) &&
	    oobject_is_equal(osic, a->keyword, b->keyword))
	{
		return osic->l_true;
	}
	return osic->l_false;
}

static struct oobject *
okarg_string(struct osic *osic, struct okarg *self)
{
	char *buffer;
	unsigned long length;

	struct oobject *keyword;
	struct oobject *argument;

	struct oobject *string;

	keyword = oobject_string(osic, self->keyword);
	argument = oobject_string(osic, self->argument);

	length = 4; /* minimal length '(, )' */
	length += ostring_length(osic, keyword);
	length += ostring_length(osic, argument);

	buffer = osic_allocator_alloc(osic, length);
	if (!buffer) {
		return NULL;
	}
	snprintf(buffer,
	         length,
	         "(%s, %s)",
	         ostring_to_cstr(osic, keyword),
	         ostring_to_cstr(osic, argument));

	string = ostring_create(osic, buffer, length);
	osic_allocator_free(osic, buffer);

	return string;
}

static struct oobject *
okarg_mark(struct osic *osic, struct okarg *self)
{
	oobject_mark(osic, self->keyword);
	oobject_mark(osic, self->argument);

	return NULL;
}

static struct oobject *
okarg_method(struct osic *osic,
             struct oobject *self,
             int method, int argc, struct oobject *argv[])
{
#define cast(a) ((struct okarg *)(a))

	switch (method) {
	case OOBJECT_METHOD_EQ:
		return okarg_eq(osic, cast(self), cast(argv[0]));

	case OOBJECT_METHOD_MARK:
		return okarg_mark(osic, cast(self));

	case OOBJECT_METHOD_STRING:
		return okarg_string(osic, cast(self));

	default:
		return oobject_default(osic, self, method, argc, argv);
	}
}

void *
okarg_create(struct osic *osic,
             struct oobject *keyword,
             struct oobject *argument)
{
	struct okarg *self;

	self = oobject_create(osic, sizeof(*self), okarg_method);
	if (self) {
		self->keyword = keyword;
		self->argument = argument;
	}

	return self;
}

struct otype *
okarg_type_create(struct osic *osic)
{
	return otype_create(osic, "karg", okarg_method, NULL);
}
