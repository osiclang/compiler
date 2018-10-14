#include "osic.h"
#include "lstring.h"
#include "lcoroutine.h"

#include <stdio.h>
#include <string.h>

static struct lobject *
lcoroutine_current(struct osic *osic,
                   struct lobject *self,
                   int argc, struct lobject *argv[])
{
	struct lcoroutine *coroutine;

	coroutine = (struct lcoroutine *)self;

	return coroutine->current;
}

static struct lobject *
lcoroutine_frame_callback(struct osic *osic,
                          struct lframe *frame,
                          struct lobject *retval)
{
	struct lcoroutine *coroutine;

	coroutine = (struct lcoroutine *)frame->self;
	if (coroutine->finished) {
		coroutine->current = retval;
	}

	return retval;
}

static struct lobject *
lcoroutine_resume(struct osic *osic,
                  struct lobject *self,
                  int argc, struct lobject *argv[])
{
	int i;
	struct lframe *frame;
	struct lcoroutine *coroutine;

	coroutine = (struct lcoroutine *)self;
	if (coroutine->finished) {
		return lobject_error_type(osic,
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
	                                     lcoroutine_frame_callback,
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

static struct lobject *
lcoroutine_transfer(struct osic *osic,
                    struct lobject *self,
                    int argc, struct lobject *argv[])
{
	int i;
	size_t size;
	struct lframe *frame;
	struct lcoroutine *coroutine;

	/* old's coroutine */
	osic_machine_pop_frame(osic); /* pop transfer's frame */
	frame = osic_machine_pop_frame(osic); /* pop caller's frame */

	if (!lobject_is_coroutine(osic, frame->callee)) {
		const char *cstr = "can't call transfer from %@";

		return lobject_error_type(osic, cstr, frame->callee);
	}

	/* save old coroutine */
	coroutine = (struct lcoroutine *)frame->callee;
	coroutine->frame = frame;
	coroutine->address = osic_machine_get_pc(osic);

	if (osic_machine_get_sp(osic) - frame->sp > 0) {
		coroutine->stacklen = osic_machine_get_sp(osic) - frame->sp;
		if (coroutine->stack) {
			osic_allocator_free(osic, coroutine->stack);
		}
		size = sizeof(struct lobject *) * coroutine->stacklen;
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
	coroutine = (struct lcoroutine *)self;
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

static struct lobject *
lcoroutine_get_attr(struct osic *osic,
                    struct lobject *self,
                    struct lobject *name)
{
	const char *cstr;

	cstr = lstring_to_cstr(osic, name);
	if (strcmp(cstr, "resume") == 0) {
		return lfunction_create(osic, name, self, lcoroutine_resume);
	}

	if (strcmp(cstr, "transfer") == 0) {
		return lfunction_create(osic, name, self, lcoroutine_transfer);
	}

	if (strcmp(cstr, "current") == 0) {
		return lfunction_create(osic, name, self, lcoroutine_current);
	}

	return osic->l_nil;
}

static struct lobject *
lcoroutine_mark(struct osic *osic, struct lcoroutine *self)
{
	int i;

	lobject_mark(osic, (struct lobject *)self->frame);
	for (i = 0; i < self->stacklen; i++) {
		lobject_mark(osic, self->stack[i]);
	}

	return NULL;
}

static struct lobject *
lcoroutine_string(struct osic *osic, struct lobject *self)
{
	char buffer[256];

	snprintf(buffer, sizeof(buffer), "<coroutine %p>", (void *)self);
	buffer[sizeof(buffer) - 1] = '\0';

	return lstring_create(osic, buffer, strlen(buffer));
}

static struct lobject *
lcoroutine_destroy(struct osic *osic, struct lcoroutine *self)
{
	osic_allocator_free(osic, self->stack);

	return NULL;
}

static struct lobject *
lcoroutine_method(struct osic *osic,
                  struct lobject *self,
                  int method, int argc, struct lobject *argv[])
{
	switch (method) {
	case LOBJECT_METHOD_GET_ATTR:
		return lcoroutine_get_attr(osic, self, argv[0]);

	case LOBJECT_METHOD_MARK:
		return lcoroutine_mark(osic, (struct lcoroutine *)self);

	case LOBJECT_METHOD_STRING:
		return lcoroutine_string(osic, self);

	case LOBJECT_METHOD_DESTROY:
		return lcoroutine_destroy(osic, (struct lcoroutine *)self);

	default:
		return lobject_default(osic, self, method, argc, argv);
	}
}

void *
lcoroutine_create(struct osic *osic, struct lframe *frame)
{
	struct lcoroutine *self;

	self = lobject_create(osic, sizeof(*self), lcoroutine_method);
	if (self) {
		self->frame = frame;
	}

	return self;
}

static struct lobject *
lcoroutine_yield(struct osic *osic,
                 struct lobject *self,
                 int argc, struct lobject *argv[])
{
	int i;
	size_t size;
	struct lframe *frame;
	struct lcoroutine *coroutine;

	/* get caller's frame */
	frame = osic_machine_get_frame(osic, osic_machine_get_fp(osic) - 1);

	if (lobject_is_coroutine(osic, frame->callee)) {
		coroutine = (struct lcoroutine *)frame->callee;
	} else {
		coroutine = lcoroutine_create(osic, frame);
		frame->callee = (struct lobject *)coroutine;
	}
	coroutine->address = osic_machine_get_pc(osic);
	coroutine->finished = 0;

	if (osic_machine_get_sp(osic) - frame->sp > 0) {
		coroutine->stacklen = osic_machine_get_sp(osic) - frame->sp;
		if (coroutine->stack) {
			osic_allocator_free(osic, coroutine->stack);
		}
		size = sizeof(struct lobject *) * coroutine->stacklen;
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
	osic_machine_push_object(osic, (struct lobject *)coroutine);

	return osic->l_nil;
}

struct ltype *
lcoroutine_type_create(struct osic *osic)
{
	char *cstr;
	struct ltype *type;
	struct lobject *name;
	struct lobject *function;

	type = ltype_create(osic, "coroutine", lcoroutine_method, NULL);
	if (type) {
		osic_add_global(osic, "coroutine", type);
	}

	cstr = "yield";
	name = lstring_create(osic, cstr, strlen(cstr));
	function = lfunction_create(osic, name, NULL, lcoroutine_yield);
	osic_add_global(osic, cstr, function);

	return type;
}
