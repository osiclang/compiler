#include "osic.h"
#include "lstring.h"
#include "lcontinuation.h"

#include <stdio.h>
#include <string.h>

static struct lobject *
lcontinuation_call(struct osic *osic,
                   struct lcontinuation *self,
                   int argc, struct lobject *argv[])
{
	int i;
	int ra;
	struct lframe *frame;

	osic_machine_set_sp(osic, -1);
	for (i = 0; i < self->stacklen; i++) {
		osic_machine_push_object(osic, self->stack[i]);
	}

	ra = osic_machine_get_ra(osic);
	osic_machine_set_fp(osic, -1);
	for (i = 0; i < self->framelen; i++) {
		frame = self->frame[i];
		osic_machine_push_frame(osic, frame);
	}
	osic_machine_set_ra(osic, ra);

	frame = osic_machine_pop_frame(osic);
	osic_machine_restore_frame(osic, frame);
	if (argc) {
		osic_machine_push_object(osic, argv[0]);
	} else {
		osic_machine_push_object(osic, osic->l_nil);
	}
	osic_machine_set_pc(osic, self->address);
	osic_machine_set_pause(osic, self->pause);

	return osic->l_nil;
}

static struct lobject *
lcontinuation_mark(struct osic *osic, struct lcontinuation *self)
{
	int i;

	if (self->value) {
		lobject_mark(osic, self->value);
	}

	for (i = 0; i < self->framelen; i++) {
		lobject_mark(osic, (struct lobject *)self->frame[i]);
	}

	for (i = 0; i < self->stacklen; i++) {
		lobject_mark(osic, self->stack[i]);
	}

	return 0;
}

static struct lobject *
lcontinuation_string(struct osic *osic, struct lcontinuation *self)
{
	char buffer[64];

	snprintf(buffer, sizeof(buffer), "<continuation %p>", (void *)self);
	buffer[sizeof(buffer) - 1] = '\0';

	return lstring_create(osic, buffer, strlen(buffer));
}

static struct lobject *
lcontinuation_destroy(struct osic *osic, struct lcontinuation *self)
{
	osic_allocator_free(osic, self->stack);
	osic_allocator_free(osic, self->frame);

	return NULL;
}

static struct lobject *
lcontinuation_method(struct osic *osic,
                     struct lobject *self,
                     int method, int argc, struct lobject *argv[])
{
#define cast(a) ((struct lcontinuation *)(a))

	switch (method) {
	case LOBJECT_METHOD_CALL:
		return lcontinuation_call(osic, cast(self), argc, argv);

	case LOBJECT_METHOD_CALLABLE:
		return osic->l_true;

	case LOBJECT_METHOD_STRING:
		return lcontinuation_string(osic, cast(self));

	case LOBJECT_METHOD_MARK:
		return lcontinuation_mark(osic, cast(self));

	case LOBJECT_METHOD_DESTROY:
		return lcontinuation_destroy(osic, cast(self));

	default:
		return lobject_default(osic, self, method, argc, argv);
	}
}

void *
lcontinuation_create(struct osic *osic)
{
	struct lcontinuation *self;

	self = lobject_create(osic, sizeof(*self), lcontinuation_method);

	return self;
}

static struct lobject *
lcontinuation_callcc(struct osic *osic,
                     struct lobject *self,
                     int argc, struct lobject *argv[])
{
	int i;
	int framelen;
	int stacklen;
	size_t size;
	struct lobject *function;
	struct lcontinuation *continuation;

	continuation = lcontinuation_create(osic);
	if (!continuation) {
		return NULL;
	}
	continuation->address = osic_machine_get_pc(osic);

	framelen = osic_machine_get_fp(osic) + 1;

	size = sizeof(struct lframe *) * framelen;
	continuation->frame = osic_allocator_alloc(osic, size);
	if (!continuation->frame) {
		return NULL;
	}
	for (i = 0; i < framelen; i++) {
		continuation->frame[i] = osic_machine_get_frame(osic, i);
	}
	continuation->framelen = framelen;

	stacklen = osic_machine_get_sp(osic) + 1;
	size = sizeof(struct lobject *) * stacklen;
	continuation->stack = osic_allocator_alloc(osic, size);
	if (!continuation->stack) {
		return NULL;
	}
	for (i = 0; i < stacklen; i++) {
		continuation->stack[i] = osic_machine_get_stack(osic, i);
	}
	continuation->stacklen = stacklen;
	continuation->pause = osic_machine_get_pause(osic);

	if (argc) {
		struct lobject *value[1];

		function = argv[0];
		value[0] = (struct lobject *)continuation;
		lobject_call(osic, function, 1, value);
	}

	return osic->l_nil;
}

struct ltype *
lcontinuation_type_create(struct osic *osic)
{
	char *cstr;
	struct ltype *type;
	struct lobject *name;
	struct lobject *function;

	type = ltype_create(osic, "continuation", lcontinuation_method, NULL);
	if (type) {
		osic_add_global(osic, "continuation", type);
	}

	cstr = "callcc";
	name = lstring_create(osic, cstr, strlen(cstr));
	function = lfunction_create(osic, name, NULL, lcontinuation_callcc);
	osic_add_global(osic, cstr, function);

	return type;
}
