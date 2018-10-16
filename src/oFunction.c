#include "osic.h"
#include "oString.h"
#include "oInteger.h"

#include <stdio.h>
#include <string.h>

static struct oobject *
ofunction_string(struct osic *osic, struct ofunction *self)
{
	char buffer[64];

	snprintf(buffer,
	         sizeof(buffer),
	         "<function '%s'>",
	         ostring_to_cstr(osic, self->name));
	buffer[sizeof(buffer) - 1] = '\0';

	return ostring_create(osic, buffer, strlen(buffer));
}

static struct oobject *
ofunction_mark(struct osic *osic, struct ofunction *self)
{
	int i;

	oobject_mark(osic, self->name);

	if (self->self) {
		oobject_mark(osic, self->self);
	}

	/* C function doesn't have frame */
	if (self->frame) {
		oobject_mark(osic, (struct oobject *)self->frame);
	}

	for (i = 0; i < self->nparams; i++) {
		oobject_mark(osic, self->params[i]);
	}

	return NULL;
}

static struct oobject *
ofunction_call(struct osic *osic,
               struct ofunction *self,
               int argc, struct oobject *argv[])
{
	struct oframe *frame;
	struct oobject *retval;
	struct oobject *exception;

	retval = osic->l_nil;
	if (self->address > 0) {
		frame = osic_machine_push_new_frame(osic,
		                                     self->self,
		                                     (struct oobject *)self,
		                                     NULL,
		                                     self->nlocals);
		if (!frame) {
			return NULL;
		}
		frame->upframe = self->frame;

		exception = osic_machine_parse_args(osic,
		                                     (struct oobject *)self,
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
		                                     (struct oobject *)self,
		                                     oframe_default_callback,
		                                     0);
		if (!frame) {
			return NULL;
		}
		retval = self->callback(osic, self->self, argc, argv);
	}

	return retval;
}

static struct oobject *
ofunction_method(struct osic *osic,
                 struct oobject *self,
                 int method, int argc, struct oobject *argv[])
{
#define cast(a) ((struct ofunction *)(a))

	switch (method) {
	case OOBJECT_METHOD_CALL:
		return ofunction_call(osic, cast(self), argc, argv);

	case OOBJECT_METHOD_CALLABLE:
		return osic->l_true;

	case OOBJECT_METHOD_STRING:
		return ofunction_string(osic, cast(self));

	case OOBJECT_METHOD_MARK:
		return ofunction_mark(osic, cast(self));

	case OOBJECT_METHOD_DESTROY:
		return NULL;

	default:
		return oobject_default(osic, self, method, argc, argv);
	}
}

void *
ofunction_create(struct osic *osic,
                 struct oobject *name,
                 struct oobject *self,
                 ofunction_call_t callback)
{
	struct ofunction *function;

	function = oobject_create(osic, sizeof(*function), ofunction_method);
	if (function) {
		function->name = name;
		function->self = self;
		function->callback = callback;
	}

	return function;
}

void *
ofunction_create_with_address(struct osic *osic,
                              struct oobject *name,
                              int define,
                              int nlocals,
                              int nparams,
                              int nvalues,
                              int address,
                              struct oobject *params[])
{
	size_t size;
	struct ofunction *self;

	size = 0;
	if (nparams > 0) {
		size = sizeof(struct oobject *) * (nparams - 1);
	}
	self = oobject_create(osic, sizeof(*self) + size, ofunction_method);
	if (self) {
		self->name = name;
		self->define = define;
		self->nlocals = nlocals;
		self->nparams = nparams;
		self->nvalues = nvalues;
		self->address = address;
		size = sizeof(struct oobject *) * nparams;
		memcpy(self->params, params, size);
	}

	return self;
}

void *
ofunction_bind(struct osic *osic,
               struct oobject *function,
               struct oobject *self)
{
	struct ofunction *oldfunction;
	struct ofunction *newfunction;

	oldfunction = (struct ofunction *)function;
	if (oldfunction->callback) {
		newfunction = ofunction_create(osic,
		                               oldfunction->name,
		                               self,
		                               oldfunction->callback);
		return newfunction;
	}

	newfunction = ofunction_create_with_address(osic,
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

struct otype *
ofunction_type_create(struct osic *osic)
{
	struct otype *type;

	type = otype_create(osic, "function", ofunction_method, NULL);
	osic_add_global(osic, "func", type);

	return type;
}
