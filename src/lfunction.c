#include "osic.h"
#include "lstring.h"
#include "linteger.h"

#include <stdio.h>
#include <string.h>

static struct lobject *
lfunction_string(struct osic *osic, struct lfunction *self)
{
	char buffer[64];

	snprintf(buffer,
	         sizeof(buffer),
	         "<function '%s'>",
	         lstring_to_cstr(osic, self->name));
	buffer[sizeof(buffer) - 1] = '\0';

	return lstring_create(osic, buffer, strlen(buffer));
}

static struct lobject *
lfunction_mark(struct osic *osic, struct lfunction *self)
{
	int i;

	lobject_mark(osic, self->name);

	if (self->self) {
		lobject_mark(osic, self->self);
	}

	/* C function doesn't have frame */
	if (self->frame) {
		lobject_mark(osic, (struct lobject *)self->frame);
	}

	for (i = 0; i < self->nparams; i++) {
		lobject_mark(osic, self->params[i]);
	}

	return NULL;
}

static struct lobject *
lfunction_call(struct osic *osic,
               struct lfunction *self,
               int argc, struct lobject *argv[])
{
	struct lframe *frame;
	struct lobject *retval;
	struct lobject *exception;

	retval = osic->l_nil;
	if (self->address > 0) {
		frame = osic_machine_push_new_frame(osic,
		                                     self->self,
		                                     (struct lobject *)self,
		                                     NULL,
		                                     self->nlocals);
		if (!frame) {
			return NULL;
		}
		frame->upframe = self->frame;

		exception = osic_machine_parse_args(osic,
		                                     (struct lobject *)self,
		                                     frame,
		                                     self->define,
		                                     self->nvalues,
		                                     self->nparams,
		                                     self->params,
		                                     argc,
		                                     argv);

		if (exception) {
			return exception;
		}
		osic_machine_set_pc(osic, self->address);
	} else {
		frame = osic_machine_push_new_frame(osic,
		                                     NULL,
		                                     (struct lobject *)self,
		                                     lframe_default_callback,
		                                     0);
		if (!frame) {
			return NULL;
		}
		retval = self->callback(osic, self->self, argc, argv);
	}

	return retval;
}

static struct lobject *
lfunction_method(struct osic *osic,
                 struct lobject *self,
                 int method, int argc, struct lobject *argv[])
{
#define cast(a) ((struct lfunction *)(a))

	switch (method) {
	case LOBJECT_METHOD_CALL:
		return lfunction_call(osic, cast(self), argc, argv);

	case LOBJECT_METHOD_CALLABLE:
		return osic->l_true;

	case LOBJECT_METHOD_STRING:
		return lfunction_string(osic, cast(self));

	case LOBJECT_METHOD_MARK:
		return lfunction_mark(osic, cast(self));

	case LOBJECT_METHOD_DESTROY:
		return NULL;

	default:
		return lobject_default(osic, self, method, argc, argv);
	}
}

void *
lfunction_create(struct osic *osic,
                 struct lobject *name,
                 struct lobject *self,
                 lfunction_call_t callback)
{
	struct lfunction *function;

	function = lobject_create(osic, sizeof(*function), lfunction_method);
	if (function) {
		function->name = name;
		function->self = self;
		function->callback = callback;
	}

	return function;
}

void *
lfunction_create_with_address(struct osic *osic,
                              struct lobject *name,
                              int define,
                              int nlocals,
                              int nparams,
                              int nvalues,
                              int address,
                              struct lobject *params[])
{
	size_t size;
	struct lfunction *self;

	size = 0;
	if (nparams > 0) {
		size = sizeof(struct lobject *) * (nparams - 1);
	}
	self = lobject_create(osic, sizeof(*self) + size, lfunction_method);
	if (self) {
		self->name = name;
		self->define = define;
		self->nlocals = nlocals;
		self->nparams = nparams;
		self->nvalues = nvalues;
		self->address = address;
		size = sizeof(struct lobject *) * nparams;
		memcpy(self->params, params, size);
	}

	return self;
}

void *
lfunction_bind(struct osic *osic,
               struct lobject *function,
               struct lobject *self)
{
	struct lfunction *oldfunction;
	struct lfunction *newfunction;

	oldfunction = (struct lfunction *)function;
	if (oldfunction->callback) {
		newfunction = lfunction_create(osic,
		                               oldfunction->name,
		                               self,
		                               oldfunction->callback);
		return newfunction;
	}

	newfunction = lfunction_create_with_address(osic,
						    oldfunction->name,
						    oldfunction->define,
						    oldfunction->nlocals,
						    oldfunction->nparams,
						    oldfunction->nvalues,
						    oldfunction->address,
						    oldfunction->params);
	if (newfunction) {
		newfunction->self = self;
		newfunction->frame = oldfunction->frame;
	}

	return newfunction;
}

struct ltype *
lfunction_type_create(struct osic *osic)
{
	struct ltype *type;

	type = ltype_create(osic, "function", lfunction_method, NULL);
	osic_add_global(osic, "func", type);

	return type;
}
