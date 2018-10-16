#include "osic.h"
#include "oArray.h"
#include "oString.h"
#include "oInteger.h"
#include "oIterator.h"

#include <stdio.h>
#include <string.h>

static struct oobject *
oarray_eq(struct osic *osic, struct oarray *a, struct oarray *b)
{
	int i;

	if (!oobject_is_array(osic, (struct oobject *)b)) {
		return osic->l_false;
	}

	if (a->count != b->count) {
		return osic->l_false;
	}

	for (i = 0; i < a->count; i++) {
		if (!oobject_is_equal(osic, a->items[i], b->items[i])) {
			return osic->l_false;
		}
	}

	return osic->l_true;
}

static struct oobject *
oarray_add(struct osic *osic, struct oarray *a, struct oarray *b)
{
	struct oobject *array;

	if (!oobject_is_array(osic, (struct oobject *)b)) {
		return osic->l_nil;
	}

	array = oarray_create(osic, a->count, a->items);
	if (array) {
		if (!oarray_append(osic, array, b->count, b->items)) {
			return NULL;
		}
	}

	return array;
}

static struct oobject *
oarray_has_item(struct osic *osic, struct oarray *a, struct oobject *b)
{
	int i;

	if (a == NULL || b == NULL) {
		return osic->l_false;
	}

	for (i = 0; i < a->count; i++) {
		if (oobject_is_equal(osic, a->items[i], b)) {
			return osic->l_true;
		}
	}

	return osic->l_false;
}

struct oobject *
oarray_get_item(struct osic *osic,
                struct oobject *self,
                long i)
{
	struct oarray *array;

	array = (struct oarray *)self;
	if (i < 0) {
		i = array->count + i;
	}
	if (i < 0 || i >= array->count) {
		return oobject_error_item(osic,
		                          "'%@' index out of range",
		                          self);
	}
	osic_collector_barrierback(osic, self, array->items[i]);

	return array->items[i];
}

struct oobject *
oarray_set_item(struct osic *osic,
                struct oobject *self,
                long i,
                struct oobject *value)
{
	struct oarray *array;

	array = (struct oarray *)self;
	if (i < 0) {
		i = array->count + i;
	}
	if (i < 0 || i >= array->count) {
		return oobject_error_item(osic,
		                          "'%@' index out of range",
		                          self);
	}
	array->items[i] = value;
	osic_collector_barrierback(osic, self, value);

	return value;
}

struct oobject *
oarray_del_item(struct osic *osic,
                struct oobject *self,
                long i)
{
	struct oarray *array;

	array = (struct oarray *)self;
	if (i < 0) {
		i = array->count + i;
	}
	if (i < 0 || i >= array->count) {
		return oobject_error_item(osic,
		                          "'%@' index out of range",
		                          self);
	}

	if (i != array->count - 1) {
		memmove(&array->items[i],
		        &array->items[i+1],
		        (array->count - i - 1) * sizeof(struct oobject *));
	}
	array->count -= 1;

	return osic->l_nil;
}

struct oobject *
oarray_append(struct osic *osic,
              struct oobject *self,
              int argc, struct oobject *argv[])
{
	int i;
	int count;
	size_t size;
	struct oarray *array;

	array = (struct oarray *)self;
	count = array->count + argc;
	if (count > array->alloc) {
		size = sizeof(struct oobject *) * count * 2;
		array->items = osic_allocator_realloc(osic,
		                                       array->items,
		                                       size);
		if (!array->items) {
			return NULL;
		}
		array->alloc = count * 2;
	}
	for (i = 0; i < argc; i++) {
		osic_collector_barrierback(osic, self, argv[i]);
		array->items[array->count + i] = argv[i];
	}
	array->count = count;

	return osic->l_nil;
}

static struct oobject *
oarray_pop(struct osic *osic,
           struct oobject *self,
           int argc, struct oobject *argv[])
{
	struct oarray *array;

	array = (struct oarray *)self;
	if (!array->count) {
		return oobject_error_item(osic, "pop() from empty list");
	}

	return array->items[--array->count];
}

static struct oobject *
oarray_iterator_next(struct osic *osic,
                     struct oobject *iterable,
                     struct oobject **context)
{
	long i;
	struct oarray *array;

	array = (struct oarray *)iterable;
	if (array == NULL) {
		return osic->l_nil;
	}

	i = ointeger_to_long(osic, *context);
	if (i >= array->count) {
		return osic->l_sentinel;
	}
	*context = ointeger_create_from_long(osic, i + 1);

	return array->items[i];
}

static struct oobject *
oarray_iterator(struct osic *osic,
                struct oobject *self,
                int argc, struct oobject *argv[])
{
	struct oobject *context;

	context = ointeger_create_from_long(osic, 0);

	return oiterator_create(osic, self, context, oarray_iterator_next);
}

static struct oobject *
oarray_get_attr(struct osic *osic,
                struct oobject *self,
                struct oobject *name)
{
	const char *cstr;

	cstr = ostring_to_cstr(osic, name);
	if (strcmp(cstr, "append") == 0) {
		return ofunction_create(osic, name, self, oarray_append);
	}

	if (strcmp(cstr, "pop") == 0) {
		return ofunction_create(osic, name, self, oarray_pop);
	}

	if (strcmp(cstr, "__iterator__") == 0) {
		return ofunction_create(osic, name, self, oarray_iterator);
	}

	return NULL;
}

static struct oobject *
oarray_get_slice(struct osic *osic,
                 struct oobject *self,
                 struct oobject *start,
                 struct oobject *stop,
                 struct oobject *step)
{
	long istart;
	long istop;
	long istep;

	struct oobject *slice;
	struct oobject *argv[1];

	slice = oarray_create(osic, 0, NULL);
	if (slice) {
		istart = ointeger_to_long(osic, start);
		if (stop == osic->l_nil) {
			istop = ((struct oarray *)self)->count;
		} else {
			istop = ointeger_to_long(osic, stop);
		}
		istep = ointeger_to_long(osic, step);
		for (; istart < istop; istart += istep) {
			argv[0] = oarray_get_item(osic, self, istart);
			if (!oarray_append(osic, slice, 1, argv)) {
				return NULL;
			}
		}
	}

	return slice;
}

static struct oobject *
oarray_string(struct osic *osic, struct oarray *self)
{
	int i;
	char *buffer;
	const char *fmt;
	unsigned long offset;
	unsigned long length;
	unsigned long maxlen;
	struct oobject *string;

	maxlen = 256;
	buffer = osic_allocator_alloc(osic, maxlen);
	if (!buffer) {
		return NULL;
	}
	offset = snprintf(buffer, sizeof(buffer), "[");
	for (i = 0; i < self->count; i++) {
		if (oobject_is_string(osic, self->items[i])) {
			if (i <  self->count - 1) {
				fmt = "'%s', ";
			} else {
				fmt = "'%s'";
			}
			string = self->items[i];
		} else {
			if (i <  self->count - 1) {
				fmt = "%s, ";
			} else {
				fmt = "%s";
			}
			string = oobject_string(osic, self->items[i]);
		}

again:
		length = snprintf(buffer + offset,
		                  maxlen - offset,
		                  fmt,
		                  ostring_to_cstr(osic, string));
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
	buffer[offset++] = ']';

	string = ostring_create(osic, buffer, offset);
	osic_allocator_free(osic, buffer);

	return string;
}

static struct oobject *
oarray_mark(struct osic *osic, struct oarray *self)
{
	int i;

	for (i = 0; i < self->count; i++) {
		oobject_mark(osic, self->items[i]);
	}

	return NULL;
}

static struct oobject *
oarray_destroy(struct osic *osic, struct oarray *self)
{
	osic_allocator_free(osic, self->items);

	return NULL;
}

static struct oobject *
oarray_method(struct osic *osic,
              struct oobject *self,
              int method, int argc, struct oobject *argv[])
{
#define cast(a) ((struct oarray *)(a))

	switch (method) {
	case OOBJECT_METHOD_EQ:
		return oarray_eq(osic, cast(self), cast(argv[0]));

	case OOBJECT_METHOD_ADD:
		return oarray_add(osic, cast(self), cast(argv[0]));

	case OOBJECT_METHOD_HAS_ITEM:
		return oarray_has_item(osic, cast(self), argv[0]);

	case OOBJECT_METHOD_GET_ITEM:
		if (oobject_is_integer(osic, argv[0])) {
			long i = ointeger_to_long(osic, argv[0]);
			return oarray_get_item(osic, self, i);
		}
		return NULL;

	case OOBJECT_METHOD_SET_ITEM:
		if (oobject_is_integer(osic, argv[0])) {
			long i = ointeger_to_long(osic, argv[0]);
			return oarray_set_item(osic, self, i, argv[1]);
		}
		return NULL;

	case OOBJECT_METHOD_DEL_ITEM:
		if (oobject_is_integer(osic, argv[0])) {
			long i = ointeger_to_long(osic, argv[0]);
			return oarray_del_item(osic, self, i);
		}
		return NULL;

	case OOBJECT_METHOD_GET_ATTR:
		return oarray_get_attr(osic, self, argv[0]);

	case OOBJECT_METHOD_GET_SLICE:
		return oarray_get_slice(osic, self, argv[0], argv[1], argv[2]);

	case OOBJECT_METHOD_STRING:
		return oarray_string(osic, cast(self));

	case OOBJECT_METHOD_LENGTH:
		return ointeger_create_from_long(osic, cast(self)->count);

	case OOBJECT_METHOD_BOOLEAN:
		if (cast(self)->count) {
			return osic->l_true;
		}
		return osic->l_false;

	case OOBJECT_METHOD_MARK:
		return oarray_mark(osic, cast(self));

	case OOBJECT_METHOD_DESTROY:
		return oarray_destroy(osic, cast(self));

	default:
		return oobject_default(osic, self, method, argc, argv);
	}
}

long
oarray_length(struct osic *osic, struct oobject *self)
{
	return ((struct oarray *)self)->count;
}

void *
oarray_create(struct osic *osic, int count, struct oobject *items[])
{
	int i;
	size_t size;
	struct oarray *self;

	self = oobject_create(osic, sizeof(*self), oarray_method);
	if (self) {
		if (count) {
			size = sizeof(struct oobject *) * (count);
			self->items = osic_allocator_alloc(osic, size);
			if (!self->items) {
				return NULL;
			}
			for (i = 0; i < count; i++) {
				self->items[i] = items[i];
			}
			self->count = count;
			self->alloc = count;
		}
	}

	return self;
}

static struct oobject *
oarray_type_method(struct osic *osic,
                   struct oobject *self,
                   int method, int argc, struct oobject *argv[])
{
	switch (method) {
	case OOBJECT_METHOD_CALL:
		return oarray_create(osic, argc, argv);

	case OOBJECT_METHOD_CALLABLE:
		return osic->l_true;

	default:
		return oobject_default(osic, self, method, argc, argv);
	}
}

struct otype *
oarray_type_create(struct osic *osic)
{
	struct otype *type;

	type = otype_create(osic, "array", oarray_method, oarray_type_method);
	if (type) {
		osic_add_global(osic, "array", type);
	}

	return type;
}
