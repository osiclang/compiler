#include "osic.h"
#include "oString.h"
#include "oInteger.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

struct oobject *
oframe_default_callback(struct osic *osic,
                        struct oframe *frame,
                        struct oobject *retval)
{
	return retval;
}

struct oobject *
oframe_get_item(struct osic *osic,
                struct oframe *frame,
                int local)
{
	assert(local < frame->nlocals);

	return frame->locals[local];
}

struct oobject *
oframe_set_item(struct osic *osic,
                struct oframe *frame,
                int local,
                struct oobject *value)
{
	assert(local < frame->nlocals);

	frame->locals[local] = value;
	osic_collector_barrier(osic, (struct oobject *)frame, value);

	return osic->l_nil;
}

static struct oobject *
oframe_string(struct osic *osic, struct oframe *self)
{
	char buffer[256];
	const char *callee;
	struct oobject *string;

	callee = "callback";
	if (self->callee) {
		string = oobject_string(osic, self->callee);
		callee = ostring_to_cstr(osic, string);
	}
	snprintf(buffer,
	         sizeof(buffer),
	         "<frame '%s': %p>",
	         callee,
	         (void *)self);
	buffer[sizeof(buffer) - 1] = '\0';

	return ostring_create(osic, buffer, strlen(buffer));
}

static struct oobject *
oframe_mark(struct osic *osic, struct oframe *self)
{
	int i;

	if (self->self) {
		oobject_mark(osic, self->self);
	}

	if (self->callee) {
		oobject_mark(osic, self->callee);
	}

	if (self->upframe) {
		oobject_mark(osic, (struct oobject *)self->upframe);
	}

	for (i = 0; i < self->nlocals; i++) {
		if (self->locals[i]) {
			oobject_mark(osic, self->locals[i]);
		}
	}

	return NULL;
}

static struct oobject *
oframe_method(struct osic *osic,
              struct oobject *self,
              int method, int argc, struct oobject *argv[])
{
#define cast(a) ((struct oframe *)(a))

	switch (method) {
	case OOBJECT_METHOD_GET_ITEM:
		if (oobject_is_integer(osic, argv[0])) {
			long i;

			i = ointeger_to_long(osic, argv[0]);
			return oframe_get_item(osic, cast(self), i);
		}
		return NULL;

	case OOBJECT_METHOD_SET_ITEM:
		if (oobject_is_integer(osic, argv[0])) {
			long i;

			i = ointeger_to_long(osic, argv[0]);
			return oframe_set_item(osic, cast(self), i, argv[1]);
		}
		return NULL;

	case OOBJECT_METHOD_STRING:
		return oframe_string(osic, cast(self));

	case OOBJECT_METHOD_MARK:
		return oframe_mark(osic, cast(self));

	case OOBJECT_METHOD_DESTROY:
		return NULL;

	default:
		return oobject_default(osic, self, method, argc, argv);
	}
}

void *
oframe_create(struct osic *osic,
              struct oobject *self,
              struct oobject *callee,
              oframe_call_t callback,
              int nlocals)
{
	size_t size;
	struct oframe *frame;

	size = 0;
	if (nlocals > 1) {
		size = sizeof(struct oobject *) * (nlocals - 1);
	}
	frame = oobject_create(osic, sizeof(*frame) + size, oframe_method);
	if (frame) {
		frame->self = self;
		frame->callee = callee;
		frame->callback = callback;
		frame->nlocals = nlocals;
	}

	return frame;
}

struct otype *
oframe_type_create(struct osic *osic)
{
	return otype_create(osic, "frame", oframe_method, NULL);
}
