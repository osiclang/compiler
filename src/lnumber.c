#include "osic.h"
#include "lnumber.h"
#include "lstring.h"
#include "linteger.h"
#include "lexception.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *
lnumber_create_from_double(struct osic *osic, double value);

static struct lobject *
lnumber_div(struct osic *osic, struct lnumber *a, struct lobject *b)
{
	double value;
	if (lobject_is_number(osic, b)) {
		value = ((struct lnumber *)b)->value;
		if (value == 0.0) {
			return lobject_error_arithmetic(osic,
			                                "divide by zero '%@/0'",
			                                a);
		}

		return lnumber_create_from_double(osic, a->value / value);
	}

	if (lobject_is_integer(osic, b)) {
		if (linteger_to_long(osic, b) == 0) {
			return lobject_error_arithmetic(osic,
			                                "divide by zero '%@/0'",
			                                a);
		}
		value = linteger_to_long(osic, b);

		return lnumber_create_from_double(osic, a->value / value);
	}

	return lobject_default(osic,
	                       (struct lobject *)a,
	                       LOBJECT_METHOD_DIV, 1, &b);
}

static struct lobject *
lnumber_destroy(struct osic *osic, struct lnumber *lnumber)
{
	return NULL;
}

static struct lobject *
lnumber_string(struct osic *osic, struct lnumber *lnumber)
{
	char buffer[256];

	snprintf(buffer, sizeof(buffer), "%f", lnumber->value);
	buffer[sizeof(buffer) - 1] = '\0';

	return lstring_create(osic, buffer, strlen(buffer));
}

static struct lobject *
lnumber_method(struct osic *osic,
               struct lobject *self,
               int method, int argc, struct lobject *argv[])
{
#define cast(a) ((struct lnumber *)(a))

#define binop(op) do {                                             \
	double value;                                              \
	if (lobject_is_number(osic, argv[0])) {                   \
		value = cast(argv[0])->value;                      \
		value = cast(self)->value op value;                \
		return lnumber_create_from_double(osic, value);   \
	}                                                          \
	if (lobject_is_integer(osic, argv[0])) {                  \
		value = (double)linteger_to_long(osic, argv[0]);  \
		value = cast(self)->value op value;                \
		return lnumber_create_from_double(osic, value);   \
	}                                                          \
	return lobject_default(osic, self, method, argc, argv);   \
} while (0)

#define cmpop(op) do {                                             \
	double value;                                              \
	if (lobject_is_number(osic, argv[0])) {                   \
		value = cast(argv[0])->value;                      \
		if (cast(self)->value op value) {                  \
			return osic->l_true;                      \
		}                                                  \
		return osic->l_false;                             \
	}                                                          \
	if (lobject_is_integer(osic, argv[0])) {                  \
		value = (double)linteger_to_long(osic, argv[0]);  \
		if (cast(self)->value op value) {                  \
			return osic->l_true;                      \
		}                                                  \
		return osic->l_false;                             \
	}                                                          \
	return lobject_default(osic, self, method, argc, argv);   \
} while (0)

	switch (method) {
	case LOBJECT_METHOD_ADD:
		binop(+);

	case LOBJECT_METHOD_SUB:
		binop(-);

	case LOBJECT_METHOD_MUL:
		binop(*);

	case LOBJECT_METHOD_DIV:
		return lnumber_div(osic, cast(self), argv[0]);

	case LOBJECT_METHOD_MOD: {
		double value;
		if (lobject_is_number(osic, argv[0])) {
			value = cast(argv[0])->value;
			value = fmod(cast(self)->value, value);
			return lnumber_create_from_double(osic, value);
		}
		if (lobject_is_integer(osic, argv[0])) {
			value = (double)linteger_to_long(osic, argv[0]);
			value = fmod(cast(self)->value, value);
			return lnumber_create_from_double(osic, value);
		}
		return lobject_default(osic, self, method, argc, argv);
	}

	case LOBJECT_METHOD_POS:
		return self;

	case LOBJECT_METHOD_NEG:
		return lnumber_create_from_double(osic, -cast(self)->value);

	case LOBJECT_METHOD_LT:
		cmpop(<);

	case LOBJECT_METHOD_LE:
		cmpop(<=);

	case LOBJECT_METHOD_EQ:
		cmpop(==);

	case LOBJECT_METHOD_NE:
		cmpop(!=);

	case LOBJECT_METHOD_GE:
		cmpop(>=);

	case LOBJECT_METHOD_GT:
		cmpop(>);

	case LOBJECT_METHOD_HASH:
		return linteger_create_from_long(osic,
		                                 (long)cast(self)->value);

	case LOBJECT_METHOD_NUMBER:
		return self;

	case LOBJECT_METHOD_BOOLEAN:
		if (cast(self)->value) {
			return osic->l_true;
		}
		return osic->l_false;

	case LOBJECT_METHOD_STRING:
		return lnumber_string(osic, cast(self));

	case LOBJECT_METHOD_INTEGER:
		return linteger_create_from_long(osic,
		                                 (long)cast(self)->value);

	case LOBJECT_METHOD_DESTROY:
		return lnumber_destroy(osic, cast(self));

	default:
		return lobject_default(osic, self, method, argc, argv);
	}
}

double
lnumber_to_double(struct osic *osic, struct lobject *self)
{
	return ((struct lnumber *)self)->value;
}

void *
lnumber_create(struct osic *osic)
{
	struct lnumber *self;

	self = lobject_create(osic, sizeof(*self), lnumber_method);

	return self;
}

void *
lnumber_create_from_long(struct osic *osic, long value)
{
	struct lnumber *self;

	self = lnumber_create(osic);
	if (self) {
		self->value = (double)value;
	}

	return self;
}

void *
lnumber_create_from_cstr(struct osic *osic, const char *value)
{
	struct lnumber *self;

	self = lnumber_create(osic);
	if (self) {
		self->value = strtod(value, NULL);
	}

	return self;
}

void *
lnumber_create_from_double(struct osic *osic, double value)
{
	struct lnumber *self;

	self = lnumber_create(osic);
	if (self) {
		self->value = value;
	}

	return self;
}

static struct lobject *
lnumber_type_method(struct osic *osic,
                    struct lobject *self,
                    int method, int argc, struct lobject *argv[])
{
	switch (method) {
	case LOBJECT_METHOD_CALL: {
		if (argc < 1) {
			return lnumber_create_from_double(osic, 0.0);
		}

		if (lobject_is_integer(osic, argv[0])) {
			double value;

			value = linteger_to_long(osic, argv[0]);
			return lnumber_create_from_double(osic, value);
		}

		if (lobject_is_string(osic, argv[0])) {
			const char *cstr;

			cstr = lstring_to_cstr(osic, argv[0]);
			return lnumber_create_from_cstr(osic, cstr);
		}

		return lobject_error_type(osic,
		                          "number() accept integer or string");
	}

	case LOBJECT_METHOD_CALLABLE:
		return osic->l_true;

	default:
		return lobject_default(osic, self, method, argc, argv);
	}
}

struct ltype *
lnumber_type_create(struct osic *osic)
{
	struct ltype *type;

	type = ltype_create(osic,
	                    "number",
	                    lnumber_method,
	                    lnumber_type_method);
	if (type) {
		osic_add_global(osic, "number", type);
	}

	return type;
}
