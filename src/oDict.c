#include "osic.h"
#include "oKarg.h"
#include "oTable.h"
#include "oString.h"
#include "oDict.h"

#include <string.h>

static struct oobject *
odict_eq(struct osic *osic,
               struct odict *a,
               struct odict *b)
{
	if (oobject_is_dictionary(osic, (struct oobject *)b)) {
		return oobject_eq(osic, a->table, b->table);
	}

	return osic->l_false;
}

static struct oobject *
odict_get_item(struct osic *osic,
                     struct odict *self,
                     struct oobject *key)
{
	return oobject_get_item(osic, self->table, key);
}

static struct oobject *
odict_has_item(struct osic *osic,
                     struct odict *self,
                     struct oobject *key)
{
	return oobject_has_item(osic, self->table, key);
}

static struct oobject *
odict_set_item(struct osic *osic,
                     struct odict *self,
                     struct oobject *key,
                     struct oobject *value)
{
	return oobject_set_item(osic, self->table, key, value);
}

static struct oobject *
odict_del_item(struct osic *osic,
                     struct odict *self,
                     struct oobject *key)
{
	return oobject_del_item(osic, self->table, key);
}

static struct oobject *
odict_map_item(struct osic *osic, struct odict *self)
{
	return oobject_map_item(osic, self->table);
}

static struct oobject *
odict_get_attr(struct osic *osic,
                     struct odict *self,
                     struct oobject *name)
{
	const char *cstr;

	cstr = ostring_to_cstr(osic, name);
	if (strcmp(cstr, "__iterator__") == 0) {
		return oobject_get_attr(osic, self->table, name);
	}

	if (strcmp(cstr, "keys") == 0) {
		return oobject_get_attr(osic, self->table, name);
	}

	return NULL;
}

static struct oobject *
odict_string(struct osic *osic, struct odict *self)
{
	return oobject_string(osic, self->table);
}

static struct oobject *
odict_mark(struct osic *osic, struct odict *self)
{
	return oobject_mark(osic, self->table);
}

static struct oobject *
odict_method(struct osic *osic,
                   struct oobject *self,
                   int method, int argc, struct oobject *argv[])
{
#define cast(a) ((struct odict *)(a))

	switch (method) {
	case OOBJECT_METHOD_EQ:
		return odict_eq(osic, cast(self), cast(argv[0]));

	case OOBJECT_METHOD_GET_ITEM:
		return odict_get_item(osic, cast(self), argv[0]);

	case OOBJECT_METHOD_HAS_ITEM:
		return odict_has_item(osic, cast(self), argv[0]);

	case OOBJECT_METHOD_SET_ITEM:
		return odict_set_item(osic,
		                            cast(self),
		                            argv[0],
		                            argv[1]);

	case OOBJECT_METHOD_DEL_ITEM:
		return odict_del_item(osic, cast(self), argv[0]);

	case OOBJECT_METHOD_MAP_ITEM:
		return odict_map_item(osic, cast(self));

	case OOBJECT_METHOD_GET_ATTR:
		return odict_get_attr(osic, cast(self), argv[0]);

	case OOBJECT_METHOD_STRING:
		return odict_string(osic, cast(self));

	case OOBJECT_METHOD_LENGTH:
		return oobject_length(osic, cast(self)->table);

	case OOBJECT_METHOD_MARK:
		return odict_mark(osic, cast(self));

	default:
		return oobject_default(osic, self, method, argc, argv);
	}
}

void *
odict_create(struct osic *osic, int count, struct oobject *items[])
{
	int i;
	struct odict *self;

	self = oobject_create(osic, sizeof(*self), odict_method);
	if (self) {
		self->table = otable_create(osic);
		if (!self->table) {
			return NULL;
		}
		for (i = 0; i < count; i += 2) {
			struct oobject *key;
			struct oobject *value;

			key = items[i];
			value = items[i+1];
			odict_set_item(osic, self, key, value);
		}
	}

	return self;
}

static struct oobject *
odict_type_method(struct osic *osic,
                       struct oobject *self,
                       int method, int argc, struct oobject *argv[])
{
	switch (method) {
	case OOBJECT_METHOD_CALL: {
		int i;
		struct okarg *karg;
		struct odict *dictionary;

		dictionary = odict_create(osic, 0, NULL);
		for (i = 0; i < argc; i++) {
			if (!oobject_is_karg(osic, argv[i])) {
				const char *fmt;

				fmt = "'%@' accept keyword arguments";
				return oobject_error_type(osic, fmt, self);
			}
			karg = (struct okarg *)argv[i];
			odict_set_item(osic,
					     dictionary,
					     karg->keyword,
					     karg->argument);
		}

		return (struct oobject *)dictionary;
	}

	case OOBJECT_METHOD_CALLABLE:
		return osic->l_true;

	default:
		return oobject_default(osic, self, method, argc, argv);
	}
}

struct otype *
odict_type_create(struct osic *osic)
{
	struct otype *type;

	type = otype_create(osic,
	                    "dictionary",
	                    odict_method,
	                    odict_type_method);
	if (type) {
		osic_add_global(osic, "dictionary", type);
	}

	return type;
}
