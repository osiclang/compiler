#include "osic.h"
#include "oNumber.h"
#include "oString.h"
#include "oInteger.h"
#include "oException.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *
onumber_create_from_double(struct osic *osic, double value);

static struct oobject *
onumber_div(struct osic *osic, struct onumber *a, struct oobject *b)
{
	double value;
	if (oobject_is_number(osic, b)) {
		value = ((struct onumber *)b)->value;
		if (value == 0.0) {
			return oobject_error_arithmetic(osic,
			                                "divide by zero '%@/0'",
			                                a);
		}

		return onumber_create_from_double(osic, a->value / value);
	}

	if (oobject_is_integer(osic, b)) {
		if (ointeger_to_long(osic, b) == 0) {
			return oobject_error_arithmetic(osic,
			                                "divide by zero '%@/0'",
			                                a);
		}
		value = ointeger_to_long(osic, b);

		return onumber_create_from_double(osic, a->value / value);
	}

	return oobject_default(osic,
	                       (struct oobject *)a,
	                       OOBJECT_METHOD_DIV, 1, &b);
}

static struct oobject *
onumber_destroy(struct osic *osic, struct onumber *onumber)
{
	return NULL;
}

static struct oobject *
onumber_string(struct osic *osic, struct onumber *onumber)
{
	char buffer[256];

	snprintf(buffer, sizeof(buffer), "%f", onumber->value);
	buffer[sizeof(buffer) - 1] = '\0';

	return ostring_create(osic, buffer, strlen(buffer));
}

static struct oobject *
onumber_method(struct osic *osic,
               struct oobject *self,
               int method, int argc, struct oobject *argv[])
{
#define cast(a) ((struct onumber *)(a))

#define binop(op) do {                                             \
	double value;                                              \
	if (oobject_is_number(osic, argv[0])) {                   \
		value = cast(argv[0])->value;                      \
		value = cast(self)->value op value;                \
		return onumber_create_from_double(osic, value);   \
	}                                                          \
	if (oobject_is_integer(osic, argv[0])) {                  \
		value = (double)ointeger_to_long(osic, argv[0]);  \
		value = cast(self)->value op value;                \
		return onumber_create_from_double(osic, value);   \
	}                                                          \
	return oobject_default(osic, self, method, argc, argv);   \
} while (0)

#define cmpop(op) do {                                             \
	double value;                                              \
	if (oobject_is_number(osic, argv[0])) {                   \
		value = cast(argv[0])->value;                      \
		if (cast(self)->value op value) {                  \
			return osic->l_true;                      \
		}                                                  \
		return osic->l_false;                             \
	}                                                          \
	if (oobject_is_integer(osic, argv[0])) {                  \
		value = (double)ointeger_to_long(osic, argv[0]);  \
		if (cast(self)->value op value) {                  \
			return osic->l_true;                      \
		}                                                  \
		return osic->l_false;                             \
	}                                                          \
	return oobject_default(osic, self, method, argc, argv);   \
} while (0)

	switch (method) {
	case OOBJECT_METHOD_ADD:
		binop(+);

	case OOBJECT_METHOD_SUB:
		binop(-);

	case OOBJECT_METHOD_MUL:
		binop(*);

	case OOBJECT_METHOD_DIV:
		return onumber_div(osic, cast(self), argv[0]);

	case OOBJECT_METHOD_MOD: {
		double value;
		if (oobject_is_number(osic, argv[0])) {
			value = cast(argv[0])->value;
			value = fmod(cast(self)->value, value);
			return onumber_create_from_double(osic, value);
		}
		if (oobject_is_integer(osic, argv[0])) {
			value = (double)ointeger_to_long(osic, argv[0]);
			value = fmod(cast(self)->value, value);
			return onumber_create_from_double(osic, value);
		}
		return oobject_default(osic, self, method, argc, argv);
	}

	case OOBJECT_METHOD_POS:
		return self;

	case OOBJECT_METHOD_NEG:
		return onumber_create_from_double(osic, -cast(self)->value);

	case OOBJECT_METHOD_LT:
		cmpop(<);

	case OOBJECT_METHOD_LE:
		cmpop(<=);

	case OOBJECT_METHOD_EQ:
		cmpop(==);

	case OOBJECT_METHOD_NE:
		cmpop(!=);

	case OOBJECT_METHOD_GE:
		cmpop(>=);

	case OOBJECT_METHOD_GT:
		cmpop(>);

	case OOBJECT_METHOD_HASH:
		return ointeger_create_from_long(osic,
		                                 (long)cast(self)->value);

	case OOBJECT_METHOD_NUMBER:
		return self;

	case OOBJECT_METHOD_BOOLEAN:
		if (cast(self)->value) {
			return osic->l_true;
		}
		return osic->l_false;

	case OOBJECT_METHOD_STRING:
		return onumber_string(osic, cast(self));

	case OOBJECT_METHOD_INTEGER:
		return ointeger_create_from_long(osic,
		                                 (long)cast(self)->value);

	case OOBJECT_METHOD_DESTROY:
		return onumber_destroy(osic, cast(self));

	default:
		return oobject_default(osic, self, method, argc, argv);
	}
}

double
onumber_to_double(struct osic *osic, struct oobject *self)
{
	return ((struct onumber *)self)->value;
}

void *
onumber_create(struct osic *osic)
{
	struct onumber *self;

	self = oobject_create(osic, sizeof(*self), onumber_method);

	return self;
}

void *
onumber_create_from_long(struct osic *osic, long value)
{
	struct onumber *self;

	self = onumber_create(osic);
	if (self) {
		self->value = (double)value;
	}

	return self;
}

void *
onumber_create_from_cstr(struct osic *osic, const char *value)
{
	struct onumber *self;

	self = onumber_create(osic);
	if (self) {
		self->value = strtod(value, NULL);
	}

	return self;
}

void *
onumber_create_from_double(struct osic *osic, double value)
{
	struct onumber *self;

	self = onumber_create(osic);
	if (self) {
		self->value = value;
	}

	return self;
}

static struct oobject *
onumber_type_method(struct osic *osic,
                    struct oobject *self,
                    int method, int argc, struct oobject *argv[])
{
	switch (method) {
	case OOBJECT_METHOD_CALL: {
		if (argc < 1) {
			return onumber_create_from_double(osic, 0.0);
		}

		if (oobject_is_integer(osic, argv[0])) {
			double value;

			value = ointeger_to_long(osic, argv[0]);
			return onumber_create_from_double(osic, value);
		}

		if (oobject_is_string(osic, argv[0])) {
			const char *cstr;

			cstr = ostring_to_cstr(osic, argv[0]);
			return onumber_create_from_cstr(osic, cstr);
		}

		return oobject_error_type(osic,
		                          "number() accept integer or string");
	}

	case OOBJECT_METHOD_CALLABLE:
		return osic->l_true;

	default:
		return oobject_default(osic, self, method, argc, argv);
	}
}

struct otype *
onumber_type_create(struct osic *osic)
{
	struct otype *type;

	type = otype_create(osic,
	                    "number",
	                    onumber_method,
	                    onumber_type_method);
	if (type) {
		osic_add_global(osic, "number", type);
	}

	return type;
}
