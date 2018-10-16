#include "osic.h"
#include "oString.h"
#include "oCoroutine.h"

#include <stdio.h>
#include <string.h>

static struct oobject *
ocoroutine_current(struct osic *osic,
                   struct oobject *self,
                   int argc, struct oobject *argv[])
{
	struct ocoroutine *coroutine;

	coroutine = (struct ocoroutine *)self;

	return coroutine->current;
}

static struct oobject *
ocoroutine_frame_callback(struct osic *osic,
                          struct oframe *frame,
                          struct oobject *retval)
{
	struct ocoroutine *coroutine;

	coroutine = (struct ocoroutine *)frame->self;
	if (coroutine->finished) {
		coroutine->current = retval;
	}

	return retval;
}

static struct oobject *
ocoroutine_resume(struct osic *osic,
                  struct oobject *self,
                  int argc, struct oobject *argv[])
{
	int i;
	struct oframe *frame;
	struct ocoroutine *coroutine;

	coroutine = (struct ocoroutine *)self;
	if (coroutine->finished) {
		return oobject_error_type(osic,
		                          "resume finished coroutine '%@'",
		                          self);
	}
	osic_machine_pop_frame(osic);

	osic_machine_store_frame(osic, coroutine->frame);
	for (i = coroutine->stacklen; i > 0; i--) {
		osic_machine_push_object(osic, coroutine->stack[i-1]);
	}

	frame = osic_machine_push_new_frame(osic,
	                                     self,
	                                     NULL,
	                                     ocoroutine_frame_callback,
	                                     0);
	if (!frame) {
		return NULL;
	}
	osic_machine_push_frame(osic, coroutine->frame);
	if (argc) {
		osic_machine_push_object(osic, argv[0]);
	} else {
		osic_machine_push_object(osic, osic->l_nil);
	}
	osic_machine_set_pc(osic, coroutine->address);
	coroutine->finished = 1;

	return osic->l_nil;
}

static struct oobject *
ocoroutine_transfer(struct osic *osic,
                    struct oobject *self,
                    int argc, struct oobject *argv[])
{
	int i;
	size_t size;
	struct oframe *frame;
	struct ocoroutine *coroutine;

	/* old's coroutine */
	osic_machine_pop_frame(osic); /* pop transfer's frame */
	frame = osic_machine_pop_frame(osic); /* pop caller's frame */

	if (!oobject_is_coroutine(osic, frame->callee)) {
		const char *cstr = "can't call transfer from %@";

		return oobject_error_type(osic, cstr, frame->callee);
	}

	/* save old coroutine */
	coroutine = (struct ocoroutine *)frame->callee;
	coroutine->frame = frame;
	coroutine->address = osic_machine_get_pc(osic);

	if (osic_machine_get_sp(osic) - frame->sp > 0) {
		coroutine->stacklen = osic_machine_get_sp(osic) - frame->sp;
		if (coroutine->stack) {
			osic_allocator_free(osic, coroutine->stack);
		}
		size = sizeof(struct oobject *) * coroutine->stacklen;
		coroutine->stack = osic_allocator_alloc(osic, size);
		if (!coroutine->stack) {
			return osic->l_out_of_memory;
		}
		memset(coroutine->stack, 0, size);
		for (i = 0; i < coroutine->stacklen; i++) {
			coroutine->stack[i] = osic_machine_pop_object(osic);
		}
	}
	osic_machine_restore_frame(osic, frame);

	/* restore new coroutine */
	coroutine = (struct ocoroutine *)self;
	for (i = coroutine->stacklen; i > 0; i--) {
		osic_machine_push_object(osic, coroutine->stack[i-1]);
	}

	osic_machine_push_frame(osic, coroutine->frame);
	if (argc) {
		osic_machine_push_object(osic, argv[0]);
	} else {
		osic_machine_push_object(osic, osic->l_nil);
	}
	osic_machine_set_pc(osic, coroutine->address);

	return osic->l_nil;
}

static struct oobject *
ocoroutine_get_attr(struct osic *osic,
                    struct oobject *self,
                    struct oobject *name)
{
	const char *cstr;

	cstr = ostring_to_cstr(osic, name);
	if (strcmp(cstr, "resume") == 0) {
		return ofunction_create(osic, name, self, ocoroutine_resume);
	}

	if (strcmp(cstr, "transfer") == 0) {
		return ofunction_create(osic, name, self, ocoroutine_transfer);
	}

	if (strcmp(cstr, "current") == 0) {
		return ofunction_create(osic, name, self, ocoroutine_current);
	}

	return osic->l_nil;
}

static struct oobject *
ocoroutine_mark(struct osic *osic, struct ocoroutine *self)
{
	int i;

	oobject_mark(osic, (struct oobject *)self->frame);
	for (i = 0; i < self->stacklen; i++) {
		oobject_mark(osic, self->stack[i]);
	}

	return NULL;
}

static struct oobject *
ocoroutine_string(struct osic *osic, struct oobject *self)
{
	char buffer[256];

	snprintf(buffer, sizeof(buffer), "<coroutine %p>", (void *)self);
	buffer[sizeof(buffer) - 1] = '\0';

	return ostring_create(osic, buffer, strlen(buffer));
}

static struct oobject *
ocoroutine_destroy(struct osic *osic, struct ocoroutine *self)
{
	osic_allocator_free(osic, self->stack);

	return NULL;
}

static struct oobject *
ocoroutine_method(struct osic *osic,
                  struct oobject *self,
                  int method, int argc, struct oobject *argv[])
{
	switch (method) {
	case OOBJECT_METHOD_GET_ATTR:
		return ocoroutine_get_attr(osic, self, argv[0]);

	case OOBJECT_METHOD_MARK:
		return ocoroutine_mark(osic, (struct ocoroutine *)self);

	case OOBJECT_METHOD_STRING:
		return ocoroutine_string(osic, self);

	case OOBJECT_METHOD_DESTROY:
		return ocoroutine_destroy(osic, (struct ocoroutine *)self);

	default:
		return oobject_default(osic, self, method, argc, argv);
	}
}

void *
ocoroutine_create(struct osic *osic, struct oframe *frame)
{
	struct ocoroutine *self;

	self = oobject_create(osic, sizeof(*self), ocoroutine_method);
	if (self) {
		self->frame = frame;
	}

	return self;
}

static struct oobject *
ocoroutine_yield(struct osic *osic,
                 struct oobject *self,
                 int argc, struct oobject *argv[])
{
	int i;
	size_t size;
	struct oframe *frame;
	struct ocoroutine *coroutine;

	/* get caller's frame */
	frame = osic_machine_get_frame(osic, osic_machine_get_fp(osic) - 1);

	if (oobject_is_coroutine(osic, frame->callee)) {
		coroutine = (struct ocoroutine *)frame->callee;
	} else {
		coroutine = ocoroutine_create(osic, frame);
		frame->callee = (struct oobject *)coroutine;
	}
	coroutine->address = osic_machine_get_pc(osic);
	coroutine->finished = 0;

	if (osic_machine_get_sp(osic) - frame->sp > 0) {
		coroutine->stacklen = osic_machine_get_sp(osic) - frame->sp;
		if (coroutine->stack) {
			osic_allocator_free(osic, coroutine->stack);
		}
		size = sizeof(struct oobject *) * coroutine->stacklen;
		coroutine->stack = osic_allocator_alloc(osic, size);
		if (!coroutine->stack) {
			return NULL;
		}
		memset(coroutine->stack, 0, size);
		for (i = 0; i < coroutine->stacklen; i++) {
			coroutine->stack[i] = osic_machine_pop_object(osic);
		}
	}

	if (argc) {
		coroutine->current = argv[0];
	} else {
		coroutine->current = osic->l_nil;
	}

	osic_machine_pop_frame(osic); /* pop yield's frame */
	osic_machine_pop_frame(osic); /* pop caller's frame */
	osic_machine_restore_frame(osic, frame);

	/* we're already pop out yield's frame, manual push return value */
	osic_machine_push_object(osic, (struct oobject *)coroutine);

	return osic->l_nil;
}

struct otype *
ocoroutine_type_create(struct osic *osic)
{
	char *cstr;
	struct otype *type;
	struct oobject *name;
	struct oobject *function;

	type = otype_create(osic, "coroutine", ocoroutine_method, NULL);
	if (type) {
		osic_add_global(osic, "coroutine", type);
	}

	cstr = "yield";
	name = ostring_create(osic, cstr, strlen(cstr));
	function = ofunction_create(osic, name, NULL, ocoroutine_yield);
	osic_add_global(osic, cstr, function);

	return type;
}
