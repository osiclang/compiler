#include "osic.h"
#include "lstring.h"
#include "linteger.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

struct lobject *
lframe_default_callback(struct osic *osic,
                        struct lframe *frame,
                        struct lobject *retval)
{
	return retval;
}

struct lobject *
lframe_get_item(struct osic *osic,
                struct lframe *frame,
                int local)
{
	assert(local < frame->nlocals);

	return frame->locals[local];
}

struct lobject *
lframe_set_item(struct osic *osic,
                struct lframe *frame,
                int local,
                struct lobject *value)
{
	assert(local < frame->nlocals);

	frame->locals[local] = value;
	osic_collector_barrier(osic, (struct lobject *)frame, value);

	return osic->l_nil;
}

static struct lobject *
lframe_string(struct osic *osic, struct lframe *self)
{
	char buffer[256];
	const char *callee;
	struct lobject *string;

	callee = "callback";
	if (self->callee) {
		string = lobject_string(osic, self->callee);
		callee = lstring_to_cstr(osic, string);
	}
	snprintf(buffer,
	         sizeof(buffer),
	         "<frame '%s': %p>",
	         callee,
	         (void *)self);
	buffer[sizeof(buffer) - 1] = '\0';

	return lstring_create(osic, buffer, strlen(buffer));
}

static struct lobject *
lframe_mark(struct osic *osic, struct lframe *self)
{
	int i;

	if (self->self) {
		lobject_mark(osic, self->self);
	}

	if (self->callee) {
		lobject_mark(osic, self->callee);
	}

	if (self->upframe) {
		lobject_mark(osic, (struct lobject *)self->upframe);
	}

	for (i = 0; i < self->nlocals; i++) {
		if (self->locals[i]) {
			lobject_mark(osic, self->locals[i]);
		}
	}

	return NULL;
}

static struct lobject *
lframe_method(struct osic *osic,
              struct lobject *self,
              int method, int argc, struct lobject *argv[])
{
#define cast(a) ((struct lframe *)(a))

	switch (method) {
	case LOBJECT_METHOD_GET_ITEM:
		if (lobject_is_integer(osic, argv[0])) {
			long i;

			i = linteger_to_long(osic, argv[0]);
			return lframe_get_item(osic, cast(self), i);
		}
		return NULL;

	case LOBJECT_METHOD_SET_ITEM:
		if (lobject_is_integer(osic, argv[0])) {
			long i;

			i = linteger_to_long(osic, argv[0]);
			return lframe_set_item(osic, cast(self), i, argv[1]);
		}
		return NULL;

	case LOBJECT_METHOD_STRING:
		return lframe_string(osic, cast(self));

	case LOBJECT_METHOD_MARK:
		return lframe_mark(osic, cast(self));

	case LOBJECT_METHOD_DESTROY:
		return NULL;

	default:
		return lobject_default(osic, self, method, argc, argv);
	}
}

void *
lframe_create(struct osic *osic,
              struct lobject *self,
              struct lobject *callee,
              lframe_call_t callback,
              int nlocals)
{
	size_t size;
	struct lframe *frame;

	size = 0;
	if (nlocals > 1) {
		size = sizeof(struct lobject *) * (nlocals - 1);
	}
	frame = lobject_create(osic, sizeof(*frame) + size, lframe_method);
	if (frame) {
		frame->self = self;
		frame->callee = callee;
		frame->callback = callback;
		frame->nlocals = nlocals;
	}

	return frame;
}

struct ltype *
lframe_type_create(struct osic *osic)
{
	return ltype_create(osic, "frame", lframe_method, NULL);
}
