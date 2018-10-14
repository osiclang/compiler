#include "osic.h"
#include "lkarg.h"
#include "ltable.h"
#include "lstring.h"
#include "ldictionary.h"

#include <string.h>

static struct lobject *
ldictionary_eq(struct osic *osic,
               struct ldictionary *a,
               struct ldictionary *b)
{
	if (lobject_is_dictionary(osic, (struct lobject *)b)) {
		return lobject_eq(osic, a->table, b->table);
	}

	return osic->l_false;
}

static struct lobject *
ldictionary_get_item(struct osic *osic,
                     struct ldictionary *self,
                     struct lobject *key)
{
	return lobject_get_item(osic, self->table, key);
}

static struct lobject *
ldictionary_has_item(struct osic *osic,
                     struct ldictionary *self,
                     struct lobject *key)
{
	return lobject_has_item(osic, self->table, key);
}

static struct lobject *
ldictionary_set_item(struct osic *osic,
                     struct ldictionary *self,
                     struct lobject *key,
                     struct lobject *value)
{
	return lobject_set_item(osic, self->table, key, value);
}

static struct lobject *
ldictionary_del_item(struct osic *osic,
                     struct ldictionary *self,
                     struct lobject *key)
{
	return lobject_del_item(osic, self->table, key);
}

static struct lobject *
ldictionary_map_item(struct osic *osic, struct ldictionary *self)
{
	return lobject_map_item(osic, self->table);
}

static struct lobject *
ldictionary_get_attr(struct osic *osic,
                     struct ldictionary *self,
                     struct lobject *name)
{
	const char *cstr;

	cstr = lstring_to_cstr(osic, name);
	if (strcmp(cstr, "__iterator__") == 0) {
		return lobject_get_attr(osic, self->table, name);
	}

	if (strcmp(cstr, "keys") == 0) {
		return lobject_get_attr(osic, self->table, name);
	}

	return NULL;
}

static struct lobject *
ldictionary_string(struct osic *osic, struct ldictionary *self)
{
	return lobject_string(osic, self->table);
}

static struct lobject *
ldictionary_mark(struct osic *osic, struct ldictionary *self)
{
	return lobject_mark(osic, self->table);
}

static struct lobject *
ldictionary_method(struct osic *osic,
                   struct lobject *self,
                   int method, int argc, struct lobject *argv[])
{
#define cast(a) ((struct ldictionary *)(a))

	switch (method) {
	case LOBJECT_METHOD_EQ:
		return ldictionary_eq(osic, cast(self), cast(argv[0]));

	case LOBJECT_METHOD_GET_ITEM:
		return ldictionary_get_item(osic, cast(self), argv[0]);

	case LOBJECT_METHOD_HAS_ITEM:
		return ldictionary_has_item(osic, cast(self), argv[0]);

	case LOBJECT_METHOD_SET_ITEM:
		return ldictionary_set_item(osic,
		                            cast(self),
		                            argv[0],
		                            argv[1]);

	case LOBJECT_METHOD_DEL_ITEM:
		return ldictionary_del_item(osic, cast(self), argv[0]);

	case LOBJECT_METHOD_MAP_ITEM:
		return ldictionary_map_item(osic, cast(self));

	case LOBJECT_METHOD_GET_ATTR:
		return ldictionary_get_attr(osic, cast(self), argv[0]);

	case LOBJECT_METHOD_STRING:
		return ldictionary_string(osic, cast(self));

	case LOBJECT_METHOD_LENGTH:
		return lobject_length(osic, cast(self)->table);

	case LOBJECT_METHOD_MARK:
		return ldictionary_mark(osic, cast(self));

	default:
		return lobject_default(osic, self, method, argc, argv);
	}
}

void *
ldictionary_create(struct osic *osic, int count, struct lobject *items[])
{
	int i;
	struct ldictionary *self;

	self = lobject_create(osic, sizeof(*self), ldictionary_method);
	if (self) {
		self->table = ltable_create(osic);
		if (!self->table) {
			return NULL;
		}
		for (i = 0; i < count; i += 2) {
			struct lobject *key;
			struct lobject *value;

			key = items[i];
			value = items[i+1];
			ldictionary_set_item(osic, self, key, value);
		}
	}

	return self;
}

static struct lobject *
ldictionary_type_method(struct osic *osic,
                       struct lobject *self,
                       int method, int argc, struct lobject *argv[])
{
	switch (method) {
	case LOBJECT_METHOD_CALL: {
		int i;
		struct lkarg *karg;
		struct ldictionary *dictionary;

		dictionary = ldictionary_create(osic, 0, NULL);
		for (i = 0; i < argc; i++) {
			if (!lobject_is_karg(osic, argv[i])) {
				const char *fmt;

				fmt = "'%@' accept keyword arguments";
				return lobject_error_type(osic, fmt, self);
			}
			karg = (struct lkarg *)argv[i];
			ldictionary_set_item(osic,
					     dictionary,
					     karg->keyword,
					     karg->argument);
		}

		return (struct lobject *)dictionary;
	}

	case LOBJECT_METHOD_CALLABLE:
		return osic->l_true;

	default:
		return lobject_default(osic, self, method, argc, argv);
	}
}

struct ltype *
ldictionary_type_create(struct osic *osic)
{
	struct ltype *type;

	type = ltype_create(osic,
	                    "dictionary",
	                    ldictionary_method,
	                    ldictionary_type_method);
	if (type) {
		osic_add_global(osic, "dictionary", type);
	}

	return type;
}
