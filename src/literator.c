#include "osic.h"
#include "larray.h"
#include "lstring.h"
#include "linteger.h"
#include "literator.h"

#include <stdio.h>
#include <limits.h>
#include <string.h>

static struct lobject *
literation_array_next_callback(struct osic *osic,
                               struct lframe *frame,
                               struct lobject *retval);

static struct lobject *
literation_array_next(struct osic *osic, struct lobject *self)
{
	struct lframe *frame;
	struct lobject *object;
	struct literator *iterator;

	iterator = (struct literator *)self;
	frame = osic_machine_push_new_frame(osic,
	                                     self,
	                                     NULL,
	                                     literation_array_next_callback,
	                                     0);
	if (!frame) {
		return NULL;
	}
	object = lobject_call_attr(osic,
	                           iterator->iterable,
	                           osic->l_next_string,
	                           0,
	                           NULL);

	return object;
}

static struct lobject *
literation_array_next_callback(struct osic *osic,
                               struct lframe *frame,
                               struct lobject *retval)
{
	long count;
	struct literator *iterator;

	iterator = (struct literator *)frame->self;
	count = larray_length(osic, iterator->context);
	if (retval == osic->l_sentinel || count == iterator->max) {
		return iterator->context;
	}

	if (iterator->next) {
		iterator->next(osic,
		               iterator->iterable,
		               &iterator->context);
	}

	if (!larray_append(osic, iterator->context, 1, &retval)) {
		return NULL;
	}
	literation_array_next(osic, frame->self);

	return osic->l_nil;
}

static struct lobject *
literation_array_callback(struct osic *osic,
                          struct lframe *frame,
                          struct lobject *retval)
{
	struct literator *iterator;

	if (!retval) {
		return lobject_error_not_iterable(osic, frame->self);
	}

	iterator = (struct literator *)frame->self;
	iterator->iterable = retval;
	iterator->context = larray_create(osic, 0, NULL);
	if (!iterator->context) {
		return NULL;
	}

	return literation_array_next(osic, frame->self);
}

static struct lobject *
literator_array(struct osic *osic,
                struct lobject *self,
                int argc, struct lobject *argv[])
{
	struct lframe *frame;
	struct lobject *object;
	struct literator *iterator;

	iterator = (struct literator *)self;
	if (lobject_is_array(osic, iterator->iterable)) {
		return iterator->iterable;
	}

	iterator->max = LONG_MAX;
	if (argc) {
		iterator->max = linteger_to_long(osic, argv[0]);
	}

	frame = osic_machine_push_new_frame(osic,
	                                     self,
	                                     NULL,
	                                     literation_array_callback,
	                                     0);
	if (!frame) {
		return NULL;
	}

	object = lobject_call_attr(osic,
	                           iterator->iterable,
	                           osic->l_iterator_string,
	                           0,
	                           NULL);

	return object;
}

static struct lobject *
literator_next(struct osic *osic,
               struct lobject *self,
               int argc, struct lobject *argv[])
{
	struct literator *iterator;

	iterator = (struct literator *)self;
	return iterator->next(osic, iterator->iterable, &iterator->context);
}

static struct lobject *
literator_get_attr(struct osic *osic,
                   struct lobject *self,
                   struct lobject *name)
{
	const char *cstr;

	cstr = lstring_to_cstr(osic, name);
	if (strcmp(cstr, "__next__") == 0) {
		return lfunction_create(osic,
		                        name,
		                        self,
		                        literator_next);
	}
	if (strcmp(cstr, "__array__") == 0) {
		return lfunction_create(osic,
		                        name,
		                        self,
		                        literator_array);
	}

	return NULL;
}

static struct lobject *
literator_mark(struct osic *osic, struct literator *self)
{
	lobject_mark(osic, self->iterable);
	lobject_mark(osic, self->context);

	return NULL;
}

static struct lobject *
literator_string(struct osic *osic, struct lobject *a)
{
	char buffer[32];

	snprintf(buffer, sizeof(buffer), "<iterator %p>", (void *)a);
	buffer[sizeof(buffer) - 1] = '\0';

	return lstring_create(osic, buffer, strlen(buffer));
}

static struct lobject *
literator_method(struct osic *osic,
                 struct lobject *self,
                 int method, int argc, struct lobject *argv[])
{
	switch (method) {
	case LOBJECT_METHOD_GET_ATTR:
		return literator_get_attr(osic, self, argv[0]);

	case LOBJECT_METHOD_STRING:
		return literator_string(osic, self);

	case LOBJECT_METHOD_MARK:
		return literator_mark(osic, (struct literator *)self);

	case LOBJECT_METHOD_DESTROY:
		return NULL;

	default:
		return lobject_default(osic, self, method, argc, argv);
	}
}

struct lobject *
literator_to_array(struct osic *osic, struct lobject *iterable, long max)
{
	int argc;
	struct lobject *argv[1];
	struct lobject *iterator;

	argc = 0;
	if (max) {
		argc = 1;
		argv[0] = linteger_create_from_long(osic, max);
	}

	if (lobject_is_iterator(osic, iterable)) {
		iterator = iterable;
	} else {
		iterator = literator_create(osic, iterable, NULL, NULL);
	}

        return lobject_call_attr(osic,
	                         iterator,
	                         osic->l_array_string,
	                         argc,
	                         argv);
}

void *
literator_create(struct osic *osic,
                 struct lobject *iterable,
                 struct lobject *context,
                 literator_next_t next)
{
	struct literator *self;

	self = lobject_create(osic, sizeof(*self), literator_method);
	if (self) {
		self->iterable = iterable;
		self->context = context;
		self->next = next;
	}

	return self;
}

struct ltype *
literator_type_create(struct osic *osic)
{
	return ltype_create(osic, "iterator", literator_method, NULL);
}
