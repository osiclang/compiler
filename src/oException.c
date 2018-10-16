#include "osic.h"
#include "oClass.h"
#include "oString.h"
#include "oException.h"

#include <stdio.h>
#include <string.h>

static struct oobject *
oexception_addtrace_attr(struct osic *osic,
                         struct oobject *self,
                         int argc, struct oobject *argv[])
{
	int i;
	struct oexception *exception;

	exception = (struct oexception *)self;
	for (i = 0; i < argc; i++) {
		exception->frame[exception->nframe++] = argv[i];
	}

	return osic->l_nil;
}

static struct oobject *
oexception_traceback_attr(struct osic *osic,
                          struct oobject *self,
                          int argc, struct oobject *argv[])
{
	int i;
	const char *cstr;
	struct oframe *frame;
	struct oobject *string;
	struct oexception *exception;

	exception = (struct oexception *)self;
	for (i = 0; i < exception->nframe; i++) {
		printf("  at ");
		frame = (struct oframe *)exception->frame[i];
		cstr = "";
		if (frame->callee) {
			string = oobject_string(osic, frame->callee);
			cstr = ostring_to_cstr(osic, string);
			printf("%s\n", cstr);
		} else {
			if (frame->self) {
				string = oobject_string(osic, frame->self);
				cstr = ostring_to_cstr(osic, string);
			}
			printf("<callback '%s'>\n", cstr);
		}
	}

	return osic->l_nil;
}

static struct oobject *
oexception_get_attr(struct osic *osic,
                    struct oobject *self,
                    struct oobject *name)
{
	const char *cstr;

	cstr = ostring_to_cstr(osic, name);
	if (strcmp(cstr, "traceback") == 0) {
		return ofunction_create(osic,
		                        name,
		                        self,
		                        oexception_traceback_attr);
	}
	if (strcmp(cstr, "addtrace") == 0) {
		return ofunction_create(osic,
		                        name,
		                        self,
		                        oexception_addtrace_attr);
	}

	return NULL;
}

static struct oobject *
oexception_string(struct osic *osic, struct oexception *self)
{
	char *buffer;
	unsigned long length;
	const char *name;
	const char *message;

	struct oclass *clazz;
	struct oobject *string;

	length = 5; /* minimal length '<()>\0' */
	if (self->clazz && oobject_is_class(osic, self->clazz)) {
		clazz = (struct oclass *)self->clazz;
		name = ostring_to_cstr(osic, clazz->name);
		length += ostring_length(osic, clazz->name);
	} else {
		name = "Exception";
		length += strlen("Exception");
	}
	message = "";
	if (self->message && oobject_is_string(osic, self->message)) {
		message = ostring_to_cstr(osic, self->message);
		length += ostring_length(osic, self->message);
	}

	buffer = osic_allocator_alloc(osic, length);
	if (!buffer) {
		return NULL;
	}
	snprintf(buffer, length, "<%s(%s)>", name, message);
	string = ostring_create(osic, buffer, length);
	osic_allocator_free(osic, buffer);

	return string;
}

static struct oobject *
oexception_mark(struct osic *osic, struct oexception *self)
{
	int i;

	if (self->clazz) {
		oobject_mark(osic, self->clazz);
	}

	if (self->message) {
		oobject_mark(osic, self->message);
	}

	for (i = 0; i < self->nframe; i++) {
		oobject_mark(osic, self->frame[i]);
	}

	return NULL;
}

static struct oobject *
oexception_method(struct osic *osic,
                  struct oobject *object,
                  int method, int argc, struct oobject *argv[])
{
#define cast(a) ((struct oexception *)(a))

	switch (method) {
	case OOBJECT_METHOD_GET_ATTR:
		return oexception_get_attr(osic, object, argv[0]);

	case OOBJECT_METHOD_STRING:
		return oexception_string(osic, cast(object));

	case OOBJECT_METHOD_MARK:
		return oexception_mark(osic, cast(object));

	default:
		return oobject_default(osic, object, method, argc, argv);
	}
}

void *
oexception_create(struct osic *osic,
                  struct oobject *message)
{
	struct oexception *self;

	self = oobject_create(osic, sizeof(*self), oexception_method);
	if (self) {
		self->message = message;
	}

	return self;
}

static struct oobject *
oexception_type_method(struct osic *osic,
                       struct oobject *self,
                       int method, int argc, struct oobject *argv[])
{
	switch (method) {
	case OOBJECT_METHOD_CALL: {
		struct oobject *message;
		struct oobject *exception;

		message = NULL;
		if (argc) {
			message = argv[0];
		}
		exception = oexception_create(osic, message);

		return exception;
	}

	case OOBJECT_METHOD_CALLABLE:
		return osic->l_true;

	default:
		return oobject_default(osic, self, method, argc, argv);
	}
}

struct otype *
oexception_type_create(struct osic *osic)
{
	struct otype *type;

	type = otype_create(osic,
	                    "Exception",
	                    oexception_method,
	                    oexception_type_method);
	if (type) {
		osic_add_global(osic, "Exception", type);
	}

	return type;
}
