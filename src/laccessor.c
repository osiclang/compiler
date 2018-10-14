#include "osic.h"
#include "laccessor.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>

static struct lobject *
laccessor_callback(struct osic *osic,
                   struct lframe *frame,
                   struct lobject *retval)
{
	struct lfunction *callee;

	/*
	 * def getter(var x) {
	 *      return def wrapper() {
	 *              self.attr
	 *      };
	 * }
	 *
	 * make function `wrapper' can access self if original function
	 * binding to instance
	 */
	callee = (struct lfunction *)frame->callee;
	if (lobject_is_function(osic, retval) &&
	    lobject_is_function(osic, (struct lobject *)callee) &&
	    callee->self)
	{
		/* binding self to accessor's wrapper */
		retval = lfunction_bind(osic, retval, callee->self);
	}

	return retval;
}

static struct lobject *
laccessor_item_callback(struct osic *osic,
                        struct lframe *frame,
                        struct lobject *retval)
{
	struct lobject *argv[1];
	struct lfunction *callee;

	callee = (struct lfunction *)frame->callee;
	if (lobject_is_function(osic, retval) &&
	    lobject_is_function(osic, (struct lobject *)callee) &&
	    callee->self)
	{
		retval = lfunction_bind(osic, retval, callee->self);
	}

	argv[0] = retval;
	lobject_call(osic, frame->self, 1, argv);

	return retval;
}

static struct lobject *
laccessor_call(struct osic *osic,
               struct laccessor *self,
               int argc, struct lobject *argv[])
{
	int i;
	struct lframe *frame;
	struct lobject *value;
	struct lobject *callable;

	/*
	 * push a frame to get the accessor's return value
	 * for binding function's self if value is a function and
	 * have `self'
	 */
	value = argv[0];
	frame = osic_machine_push_new_frame(osic,
	                                     NULL,
	                                     value,
	                                     laccessor_callback,
	                                     0);
	if (!frame) {
		return NULL;
	}

	callable = NULL;

	/* bind setter and getter */
	for (i = 0; i < self->count; i++) {
		callable = self->items[i];

		/* binding self to bare function */
		if (self->self &&
		    lobject_is_function(osic, callable) &&
		    !((struct lfunction *)callable)->self)
		{
			callable = lfunction_bind(osic, callable, self->self);
			if (!callable) {
				return NULL;
			}
		}

		/*
		 * make frame call getter and setter, last is direct call
		 * don't need a frame
		 */
		if (i == self->count - 1) {
			break;
		}
		frame = osic_machine_push_new_frame(osic,
		                                     callable,
		                                     value,
		                                     laccessor_item_callback,
		                                     0);
		if (!frame) {
			return NULL;
		}
	}

	return lobject_call(osic, callable, argc, argv);
}

static struct lobject *
laccessor_mark(struct osic *osic, struct laccessor *self)
{
	int i;

	if (self->self) {
		lobject_mark(osic, self->self);
	}

	for (i = 0; i < self->count; i++) {
		lobject_mark(osic, self->items[i]);
	}

	return NULL;
}

static struct lobject *
laccessor_method(struct osic *osic,
                 struct lobject *self,
                 int method, int argc, struct lobject *argv[])
{
#define cast(a) ((struct laccessor *)(a))

	switch (method) {
	case LOBJECT_METHOD_CALL:
		return laccessor_call(osic, cast(self), argc, argv);

	case LOBJECT_METHOD_MARK:
		return laccessor_mark(osic, cast(self));

	default:
		return lobject_default(osic, self, method, argc, argv);
	}
}

void *
laccessor_create(struct osic *osic, int count, struct lobject *items[])
{
	size_t size;
	struct laccessor *self;

	assert(count);
	assert(items);

	assert(count > 0);
	size = sizeof(struct lobject *) * (count - 1);

	self = lobject_create(osic, sizeof(*self) + size, laccessor_method);
	if (self) {
		self->count = count;
		size = sizeof(struct lobject *) * count;
		memcpy(self->items, items, size);
	}

	return self;
}
