#include "osic.h"
#include "oAccessor.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>

static struct oobject *
oaccessor_callback(struct osic *osic,
                   struct oframe *frame,
                   struct oobject *retval)
{
	struct ofunction *callee;

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
	callee = (struct ofunction *)frame->callee;
	if (oobject_is_function(osic, retval) &&
	    oobject_is_function(osic, (struct oobject *)callee) &&
	    callee->self)
	{
		/* binding self to accessor's wrapper */
		retval = ofunction_bind(osic, retval, callee->self);
	}

	return retval;
}

static struct oobject *
oaccessor_item_callback(struct osic *osic,
                        struct oframe *frame,
                        struct oobject *retval)
{
	struct oobject *argv[1];
	struct ofunction *callee;

	callee = (struct ofunction *)frame->callee;
	if (oobject_is_function(osic, retval) &&
	    oobject_is_function(osic, (struct oobject *)callee) &&
	    callee->self)
	{
		retval = ofunction_bind(osic, retval, callee->self);
	}

	argv[0] = retval;
	oobject_call(osic, frame->self, 1, argv);

	return retval;
}

static struct oobject *
oaccessor_call(struct osic *osic,
               struct oaccessor *self,
               int argc, struct oobject *argv[])
{
	int i;
	struct oframe *frame;
	struct oobject *value;
	struct oobject *callable;

	/*
	 * push a frame to get the accessor's return value
	 * for binding function's self if value is a function and
	 * have `self'
	 */
	value = argv[0];
	frame = osic_machine_push_new_frame(osic,
	                                     NULL,
	                                     value,
	                                     oaccessor_callback,
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
		    oobject_is_function(osic, callable) &&
		    !((struct ofunction *)callable)->self)
		{
			callable = ofunction_bind(osic, callable, self->self);
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
		                                     oaccessor_item_callback,
		                                     0);
		if (!frame) {
			return NULL;
		}
	}

	return oobject_call(osic, callable, argc, argv);
}

static struct oobject *
oaccessor_mark(struct osic *osic, struct oaccessor *self)
{
	int i;

	if (self->self) {
		oobject_mark(osic, self->self);
	}

	for (i = 0; i < self->count; i++) {
		oobject_mark(osic, self->items[i]);
	}

	return NULL;
}

static struct oobject *
oaccessor_method(struct osic *osic,
                 struct oobject *self,
                 int method, int argc, struct oobject *argv[])
{
#define cast(a) ((struct oaccessor *)(a))

	switch (method) {
	case OOBJECT_METHOD_CALL:
		return oaccessor_call(osic, cast(self), argc, argv);

	case OOBJECT_METHOD_MARK:
		return oaccessor_mark(osic, cast(self));

	default:
		return oobject_default(osic, self, method, argc, argv);
	}
}

void *
oaccessor_create(struct osic *osic, int count, struct oobject *items[])
{
	size_t size;
	struct oaccessor *self;

	assert(count);
	assert(items);

	assert(count > 0);
	size = sizeof(struct oobject *) * (count - 1);

	self = oobject_create(osic, sizeof(*self) + size, oaccessor_method);
	if (self) {
		self->count = count;
		size = sizeof(struct oobject *) * count;
		memcpy(self->items, items, size);
	}

	return self;
}
