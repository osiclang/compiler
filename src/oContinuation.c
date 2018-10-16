#include "osic.h"
#include "oString.h"
#include "oContinuation.h"

#include <stdio.h>
#include <string.h>

static struct oobject *
ocontinuation_call(struct osic *osic,
                   struct ocontinuation *self,
                   int argc, struct oobject *argv[])
{
	int i;
	int ra;
	struct oframe *frame;

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

static struct oobject *
ocontinuation_mark(struct osic *osic, struct ocontinuation *self)
{
	int i;

	if (self->value) {
		oobject_mark(osic, self->value);
	}

	for (i = 0; i < self->framelen; i++) {
		oobject_mark(osic, (struct oobject *)self->frame[i]);
	}

	for (i = 0; i < self->stacklen; i++) {
		oobject_mark(osic, self->stack[i]);
	}

	return 0;
}

static struct oobject *
ocontinuation_string(struct osic *osic, struct ocontinuation *self)
{
	char buffer[64];

	snprintf(buffer, sizeof(buffer), "<continuation %p>", (void *)self);
	buffer[sizeof(buffer) - 1] = '\0';

	return ostring_create(osic, buffer, strlen(buffer));
}

static struct oobject *
ocontinuation_destroy(struct osic *osic, struct ocontinuation *self)
{
	osic_allocator_free(osic, self->stack);
	osic_allocator_free(osic, self->frame);

	return NULL;
}

static struct oobject *
ocontinuation_method(struct osic *osic,
                     struct oobject *self,
                     int method, int argc, struct oobject *argv[])
{
#define cast(a) ((struct ocontinuation *)(a))

	switch (method) {
	case OOBJECT_METHOD_CALL:
		return ocontinuation_call(osic, cast(self), argc, argv);

	case OOBJECT_METHOD_CALLABLE:
		return osic->l_true;

	case OOBJECT_METHOD_STRING:
		return ocontinuation_string(osic, cast(self));

	case OOBJECT_METHOD_MARK:
		return ocontinuation_mark(osic, cast(self));

	case OOBJECT_METHOD_DESTROY:
		return ocontinuation_destroy(osic, cast(self));

	default:
		return oobject_default(osic, self, method, argc, argv);
	}
}

void *
ocontinuation_create(struct osic *osic)
{
	struct ocontinuation *self;

	self = oobject_create(osic, sizeof(*self), ocontinuation_method);

	return self;
}

static struct oobject *
ocontinuation_callcc(struct osic *osic,
                     struct oobject *self,
                     int argc, struct oobject *argv[])
{
	int i;
	int framelen;
	int stacklen;
	size_t size;
	struct oobject *function;
	struct ocontinuation *continuation;

	continuation = ocontinuation_create(osic);
	if (!continuation) {
		return NULL;
	}
	continuation->address = osic_machine_get_pc(osic);

	framelen = osic_machine_get_fp(osic) + 1;

	size = sizeof(struct oframe *) * framelen;
	continuation->frame = osic_allocator_alloc(osic, size);
	if (!continuation->frame) {
		return NULL;
	}
	for (i = 0; i < framelen; i++) {
		continuation->frame[i] = osic_machine_get_frame(osic, i);
	}
	continuation->framelen = framelen;

	stacklen = osic_machine_get_sp(osic) + 1;
	size = sizeof(struct oobject *) * stacklen;
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
		struct oobject *value[1];

		function = argv[0];
		value[0] = (struct oobject *)continuation;
		oobject_call(osic, function, 1, value);
	}

	return osic->l_nil;
}

struct otype *
ocontinuation_type_create(struct osic *osic)
{
	char *cstr;
	struct otype *type;
	struct oobject *name;
	struct oobject *function;

	type = otype_create(osic, "continuation", ocontinuation_method, NULL);
	if (type) {
		osic_add_global(osic, "continuation", type);
	}

	cstr = "callcc";
	name = ostring_create(osic, cstr, strlen(cstr));
	function = ofunction_create(osic, name, NULL, ocontinuation_callcc);
	osic_add_global(osic, cstr, function);

	return type;
}
