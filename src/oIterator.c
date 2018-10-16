#include "osic.h"
#include "oArray.h"
#include "oString.h"
#include "oInteger.h"
#include "oIterator.h"

#include <stdio.h>
#include <limits.h>
#include <string.h>

static struct oobject *
oiterator_array_next_callback(struct osic *osic,
                               struct oframe *frame,
                               struct oobject *retval);

static struct oobject *
oiterator_array_next(struct osic *osic, struct oobject *self)
{
	struct oframe *frame;
	struct oobject *object;
	struct oiterator *iterator;

	iterator = (struct oiterator *)self;
	frame = osic_machine_push_new_frame(osic,
	                                     self,
	                                     NULL,
	                                     oiterator_array_next_callback,
	                                     0);
	if (!frame) {
		return NULL;
	}
	object = oobject_call_attr(osic,
	                           iterator->iterable,
	                           osic->l_next_string,
	                           0,
	                           NULL);

	return object;
}

static struct oobject *
oiterator_array_next_callback(struct osic *osic,
                               struct oframe *frame,
                               struct oobject *retval)
{
	long count;
	struct oiterator *iterator;

	iterator = (struct oiterator *)frame->self;
	count = oarray_length(osic, iterator->context);
	if (retval == osic->l_sentinel || count == iterator->max) {
		return iterator->context;
	}

	if (iterator->next) {
		iterator->next(osic,
		               iterator->iterable,
		               &iterator->context);
	}

	if (!oarray_append(osic, iterator->context, 1, &retval)) {
		return NULL;
	}
	oiterator_array_next(osic, frame->self);

	return osic->l_nil;
}

static struct oobject *
oiterator_array_callback(struct osic *osic,
                          struct oframe *frame,
                          struct oobject *retval)
{
	struct oiterator *iterator;

	if (!retval) {
		return oobject_error_not_iterable(osic, frame->self);
	}

	iterator = (struct oiterator *)frame->self;
	iterator->iterable = retval;
	iterator->context = oarray_create(osic, 0, NULL);
	if (!iterator->context) {
		return NULL;
	}

	return oiterator_array_next(osic, frame->self);
}

static struct oobject *
oiterator_array(struct osic *osic,
                struct oobject *self,
                int argc, struct oobject *argv[])
{
	struct oframe *frame;
	struct oobject *object;
	struct oiterator *iterator;

	iterator = (struct oiterator *)self;
	if (oobject_is_array(osic, iterator->iterable)) {
		return iterator->iterable;
	}

	iterator->max = LONG_MAX;
	if (argc) {
		iterator->max = ointeger_to_long(osic, argv[0]);
	}

	frame = osic_machine_push_new_frame(osic,
	                                     self,
	                                     NULL,
	                                     oiterator_array_callback,
	                                     0);
	if (!frame) {
		return NULL;
	}

	object = oobject_call_attr(osic,
	                           iterator->iterable,
	                           osic->l_iterator_string,
	                           0,
	                           NULL);

	return object;
}

static struct oobject *
oiterator_next(struct osic *osic,
               struct oobject *self,
               int argc, struct oobject *argv[])
{
	struct oiterator *iterator;

	iterator = (struct oiterator *)self;
	return iterator->next(osic, iterator->iterable, &iterator->context);
}

static struct oobject *
oiterator_get_attr(struct osic *osic,
                   struct oobject *self,
                   struct oobject *name)
{
	const char *cstr;

	cstr = ostring_to_cstr(osic, name);
	if (strcmp(cstr, "__next__") == 0) {
		return ofunction_create(osic,
		                        name,
		                        self,
		                        oiterator_next);
	}
	if (strcmp(cstr, "__array__") == 0) {
		return ofunction_create(osic,
		                        name,
		                        self,
		                        oiterator_array);
	}

	return NULL;
}

static struct oobject *
oiterator_mark(struct osic *osic, struct oiterator *self)
{
	oobject_mark(osic, self->iterable);
	oobject_mark(osic, self->context);

	return NULL;
}

static struct oobject *
oiterator_string(struct osic *osic, struct oobject *a)
{
	char buffer[32];

	snprintf(buffer, sizeof(buffer), "<iterator %p>", (void *)a);
	buffer[sizeof(buffer) - 1] = '\0';

	return ostring_create(osic, buffer, strlen(buffer));
}

static struct oobject *
oiterator_method(struct osic *osic,
                 struct oobject *self,
                 int method, int argc, struct oobject *argv[])
{
	switch (method) {
	case OOBJECT_METHOD_GET_ATTR:
		return oiterator_get_attr(osic, self, argv[0]);

	case OOBJECT_METHOD_STRING:
		return oiterator_string(osic, self);

	case OOBJECT_METHOD_MARK:
		return oiterator_mark(osic, (struct oiterator *)self);

	case OOBJECT_METHOD_DESTROY:
		return NULL;

	default:
		return oobject_default(osic, self, method, argc, argv);
	}
}

struct oobject *
oiterator_to_array(struct osic *osic, struct oobject *iterable, long max)
{
	int argc;
	struct oobject *argv[1];
	struct oobject *iterator;

	argc = 0;
	if (max) {
		argc = 1;
		argv[0] = ointeger_create_from_long(osic, max);
	}

	if (oobject_is_iterator(osic, iterable)) {
		iterator = iterable;
	} else {
		iterator = oiterator_create(osic, iterable, NULL, NULL);
	}

        return oobject_call_attr(osic,
	                         iterator,
	                         osic->l_array_string,
	                         argc,
	                         argv);
}

void *
oiterator_create(struct osic *osic,
                 struct oobject *iterable,
                 struct oobject *context,
                 oiterator_next_t next)
{
	struct oiterator *self;

	self = oobject_create(osic, sizeof(*self), oiterator_method);
	if (self) {
		self->iterable = iterable;
		self->context = context;
		self->next = next;
	}

	return self;
}

struct otype *
oiterator_type_create(struct osic *osic)
{
	return otype_create(osic, "iterator", oiterator_method, NULL);
}
