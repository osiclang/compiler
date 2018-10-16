#include "osic.h"
#include "hash.h"
#include "table.h"
#include "oTable.h"
#include "oArray.h"
#include "oString.h"
#include "oInteger.h"

#include <stdio.h>
#include <string.h>

static int
table_cmp(struct osic *osic, void *a, void *b)
{
	return oobject_is_equal(osic, a, b);
}

static unsigned long
table_hash(struct osic *osic, void *key)
{
	struct oobject *hash;

	hash = oobject_method_call(osic, key, OOBJECT_METHOD_HASH, 0, NULL);
	if (!hash) {
		return 0;
	}

	return (unsigned long)ointeger_to_long(osic, hash);
}

static struct oobject *
otable_eq(struct osic *osic, struct otable *a, struct otable *b)
{
	int i;
	struct slot *items;
	struct oobject *value;

	if (a->object.l_method != b->object.l_method) {
		return osic->l_false;
	}

	if (a->count != b->count) {
		return osic->l_false;
	}

	items = a->items;
	for (i = 0; i < a->length; i++) {
		if (items[i].key == NULL ||
		    items[i].key == osic->l_sentinel ||
		    items[i].value == osic->l_sentinel)
		{
			continue;
		}

		value = table_search(osic,
		                     items[i].key,
		                     b->items,
		                     b->length,
		                     table_cmp,
		                     table_hash);
		if (!value) {
			return osic->l_false;
		}

		if (!oobject_is_equal(osic, items[i].value, value)) {
			return osic->l_false;
		}
	}

	return osic->l_true;
}

static struct oobject *
otable_map_item(struct osic *osic, struct otable *self)
{
	int i;
	int j;

	int count;
	struct oobject *array;
	struct oobject **map;
	struct oobject *item;
	struct oobject *kv[2];
	struct slot *items;

	j = 0;
	count = self->count;
	items = self->items;
	map = osic_allocator_alloc(osic, sizeof(struct oobject *) * count);
	if (!map) {
		return osic->l_out_of_memory;
	}
	for (i = 0; i < self->length; i++) {
		if (items[i].key == NULL ||
		    items[i].key == osic->l_sentinel ||
		    items[i].value == osic->l_sentinel)
		{
			continue;
		}

		kv[0] = items[i].key;
		kv[1] = items[i].value;

		item = oarray_create(osic, 2, kv);
		if (!item) {
			return NULL;
		}
		map[j++] = item;
	}

	array = oarray_create(osic, count, map);
	osic_allocator_free(osic, map);

	return array;
}

static struct oobject *
otable_get_item(struct osic *osic,
                struct otable *self,
                struct oobject *name)
{
	return table_search(osic,
	                    name,
	                    self->items,
	                    self->length,
	                    table_cmp,
	                    table_hash);
}

static struct oobject *
otable_has_item(struct osic *osic,
                struct otable *self,
                struct oobject *name)
{
	struct oobject *value;

	value =  table_search(osic,
	                      name,
	                      self->items,
	                      self->length,
	                      table_cmp,
	                      table_hash);
	if (value) {
		return osic->l_true;
	}
	return osic->l_false;
}

static struct oobject *
otable_set_item(struct osic *osic,
                struct otable *self,
                struct oobject *name,
                struct oobject *value)
{
	int count;
	int length;
	size_t size;
	struct slot *items;

	count = table_insert(osic,
	                     name,
	                     value,
	                     self->items,
	                     self->length,
	                     table_cmp,
	                     table_hash);

	if (count) {
		self->count += 1;
	}

	if (TABLE_LOAD_FACTOR(self->count) > self->length) {
		count = self->count;
		length = table_size(osic, TABLE_GROW_FACTOR(self->length));

		size = sizeof(struct slot) * length;
		items = osic_allocator_alloc(osic, size);
		if (!items) {
			return NULL;
		}
		memset(items, 0, size);

		table_rehash(osic,
		             self->items,
		             self->length,
		             items,
		             length,
		             table_cmp,
		             table_hash);
		osic_allocator_free(osic, self->items);
		self->items = items;
		self->count = count;
		self->length = length;
	}

	return osic->l_nil;
}

static struct oobject *
otable_del_item(struct osic *osic,
                struct otable *self,
                struct oobject *name)
{
	struct oobject *value;

	value = table_delete(osic,
	                     name,
	                     self->items,
	                     self->length,
	                     table_cmp,
	                     table_hash);
	if (value) {
		self->count -= 1;
	}

	return value;
}

static struct oobject *
otable_keys(struct osic *osic, struct otable *self)
{
	int i;
	struct slot *items;
	struct oobject *array;
	struct oobject *value;

	items = self->items;
	array = oarray_create(osic, 0, NULL);
	if (!array) {
		return NULL;
	}
	for (i = 0; i < self->length; i++) {
		if (items[i].key == NULL ||
		    items[i].key == osic->l_sentinel ||
		    items[i].value == osic->l_sentinel)
		{
			continue;
		}
		value = items[i].key;
		if (!oarray_append(osic, array, 1, &value)) {
			return NULL;
		}
	}

	return array;
}

static struct oobject *
otable_get_keys_attr(struct osic *osic,
                     struct oobject *self,
                     int argc, struct oobject *argv[])
{
	struct oobject *array;

	array = otable_keys(osic, (struct otable *)self);
	if (!array) {
		return osic->l_out_of_memory;
	}

	return array;
}

static struct oobject *
otable_get_attr(struct osic *osic,
                struct otable *self, struct oobject *name)
{
	const char *cstr;

	cstr = ostring_to_cstr(osic, name);
	if (strcmp(cstr, "__iterator__") == 0) {
		struct oobject *keys;
		keys = otable_keys(osic, self);

		return oobject_get_attr(osic, keys, name);
	}
	if (strcmp(cstr, "keys") == 0) {
		return ofunction_create(osic,
		                        name,
		                        (struct oobject *)self,
		                        otable_get_keys_attr);
	}

	return NULL;
}

static struct oobject *
otable_string(struct osic *osic, struct otable *self)
{
	int i;
	int count;
	char *buffer;
	const char *fmt;
	unsigned long offset;
	unsigned long length;
	unsigned long maxlen;

	struct slot *items;
	struct oobject *string;

	struct oobject *key;
	struct oobject *value;

	count = 0;
	items = self->items;
	maxlen = 256;
	buffer = osic_allocator_alloc(osic, maxlen);
	if (!buffer) {
		return NULL;
	}
	offset = snprintf(buffer, sizeof(buffer), "{");
	for (i = 0; i < self->length; i++) {
		if (items[i].key == NULL ||
		    items[i].key == osic->l_sentinel ||
		    items[i].value == osic->l_sentinel)
		{
			continue;
		}

		count += 1;
		if (oobject_is_string(osic, items[i].key) &&
		    oobject_is_string(osic, items[i].value))
		{
			key = items[i].key;
			value = items[i].value;
			if (count < self->count) {
				fmt = "'%s': '%s', ";
			} else {
				fmt = "'%s': '%s'";
			}
		} else if (oobject_is_string(osic, items[i].key)) {
			key = items[i].key;
			value = oobject_string(osic, items[i].value);
			if (count < self->count) {
				fmt = "'%s': %s, ";
			} else {
				fmt = "'%s': %s";
			}
		} else if (oobject_is_string(osic, items[i].value)) {
			key = oobject_string(osic, items[i].key);
			value = items[i].value;
			if (count < self->count) {
				fmt = "%s: '%s', ";
			} else {
				fmt = "%s: '%s'";
			}
		} else {
			key = oobject_string(osic, items[i].key);
			value = oobject_string(osic, items[i].value);
			if (count < self->count) {
				fmt = "%s: %s, ";
			} else {
				fmt = "%s: %s";
			}
		}

again:
		length = snprintf(buffer + offset,
		                  maxlen - offset,
		                  fmt,
		                  ostring_to_cstr(osic, key),
		                  ostring_to_cstr(osic, value));
		if (offset + length >= maxlen - 1) {
			maxlen = (offset + length) * 2;
			buffer = osic_allocator_realloc(osic, buffer, maxlen);
			if (!buffer) {
				return NULL;
			}

			goto again;
		} else {
			offset += length;
		}
	}
	buffer[offset++] = '}';

	string = ostring_create(osic, buffer, offset);
	osic_allocator_free(osic, buffer);

	return string;
}

static struct oobject *
otable_mark(struct osic *osic, struct otable *self)
{
	int i;
	struct slot *items;

	items = self->items;
	for (i = 0; i < self->length; i++) {
		if (items[i].key == NULL ||
		    items[i].key == osic->l_sentinel ||
		    items[i].value == osic->l_sentinel)
		{
			continue;
		}
		oobject_mark(osic, items[i].key);
		oobject_mark(osic, items[i].value);
	}

	return NULL;
}

static struct oobject *
otable_destroy(struct osic *osic, struct otable *self)
{
	osic_allocator_free(osic, self->items);

	return NULL;
}

static struct oobject *
otable_method(struct osic *osic,
              struct oobject *self,
              int method, int argc, struct oobject *argv[])
{
#define cast(a) ((struct otable *)(a))

	switch (method) {
	case OOBJECT_METHOD_EQ:
		return otable_eq(osic, cast(self), cast(argv[0]));

	case OOBJECT_METHOD_GET_ITEM:
		return otable_get_item(osic, cast(self), argv[0]);

	case OOBJECT_METHOD_HAS_ITEM:
		return otable_has_item(osic, cast(self), argv[0]);

	case OOBJECT_METHOD_SET_ITEM:
		return otable_set_item(osic, cast(self), argv[0], argv[1]);

	case OOBJECT_METHOD_DEL_ITEM:
		return otable_del_item(osic, cast(self), argv[0]);

	case OOBJECT_METHOD_MAP_ITEM:
		return otable_map_item(osic, cast(self));

	case OOBJECT_METHOD_GET_ATTR:
		return otable_get_attr(osic, cast(self), argv[0]);

	case OOBJECT_METHOD_STRING:
		return otable_string(osic, cast(self));

	case OOBJECT_METHOD_LENGTH:
		return ointeger_create_from_long(osic, cast(self)->count);

	case OOBJECT_METHOD_MARK:
		return otable_mark(osic, cast(self));

	case OOBJECT_METHOD_DESTROY:
		return otable_destroy(osic, cast(self));

	default:
		return oobject_default(osic, self, method, argc, argv);
	}
}

void *
otable_create(struct osic *osic)
{
	size_t size;
	struct otable *self;

	self = oobject_create(osic, sizeof(*self), otable_method);
	if (self) {
		size = sizeof(struct slot) * 3;
		self->items = osic_allocator_alloc(osic, size);
		if (!self->items) {
			return NULL;
		}
		memset(self->items, 0, size);
		self->length = 3;
	}

	return self;
}

struct otype *
otable_type_create(struct osic *osic)
{
	return otype_create(osic, "table", otable_method, NULL);
}
