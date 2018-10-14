#include "osic.h"
#include "lclass.h"
#include "lstring.h"
#include "lexception.h"

#include <stdio.h>
#include <string.h>

static struct lobject *
lexception_addtrace_attr(struct osic *osic,
                         struct lobject *self,
                         int argc, struct lobject *argv[])
{
	int i;
	struct lexception *exception;

	exception = (struct lexception *)self;
	for (i = 0; i < argc; i++) {
		exception->frame[exception->nframe++] = argv[i];
	}

	return osic->l_nil;
}

static struct lobject *
lexception_traceback_attr(struct osic *osic,
                          struct lobject *self,
                          int argc, struct lobject *argv[])
{
	int i;
	const char *cstr;
	struct lframe *frame;
	struct lobject *string;
	struct lexception *exception;

	exception = (struct lexception *)self;
	for (i = 0; i < exception->nframe; i++) {
		printf("  at ");
		frame = (struct lframe *)exception->frame[i];
		cstr = "";
		if (frame->callee) {
			string = lobject_string(osic, frame->callee);
			cstr = lstring_to_cstr(osic, string);
			printf("%s\n", cstr);
		} else {
			if (frame->self) {
				string = lobject_string(osic, frame->self);
				cstr = lstring_to_cstr(osic, string);
			}
			printf("<callback '%s'>\n", cstr);
		}
	}

	return osic->l_nil;
}

static struct lobject *
lexception_get_attr(struct osic *osic,
                    struct lobject *self,
                    struct lobject *name)
{
	const char *cstr;

	cstr = lstring_to_cstr(osic, name);
	if (strcmp(cstr, "traceback") == 0) {
		return lfunction_create(osic,
		                        name,
		                        self,
		                        lexception_traceback_attr);
	}
	if (strcmp(cstr, "addtrace") == 0) {
		return lfunction_create(osic,
		                        name,
		                        self,
		                        lexception_addtrace_attr);
	}

	return NULL;
}

static struct lobject *
lexception_string(struct osic *osic, struct lexception *self)
{
	char *buffer;
	unsigned long length;
	const char *name;
	const char *message;

	struct lclass *clazz;
	struct lobject *string;

	length = 5; /* minimal length '<()>\0' */
	if (self->clazz && lobject_is_class(osic, self->clazz)) {
		clazz = (struct lclass *)self->clazz;
		name = lstring_to_cstr(osic, clazz->name);
		length += lstring_length(osic, clazz->name);
	} else {
		name = "Exception";
		length += strlen("Exception");
	}
	message = "";
	if (self->message && lobject_is_string(osic, self->message)) {
		message = lstring_to_cstr(osic, self->message);
		length += lstring_length(osic, self->message);
	}

	buffer = osic_allocator_alloc(osic, length);
	if (!buffer) {
		return NULL;
	}
	snprintf(buffer, length, "<%s(%s)>", name, message);
	string = lstring_create(osic, buffer, length);
	osic_allocator_free(osic, buffer);

	return string;
}

static struct lobject *
lexception_mark(struct osic *osic, struct lexception *self)
{
	int i;

	if (self->clazz) {
		lobject_mark(osic, self->clazz);
	}

	if (self->message) {
		lobject_mark(osic, self->message);
	}

	for (i = 0; i < self->nframe; i++) {
		lobject_mark(osic, self->frame[i]);
	}

	return NULL;
}

static struct lobject *
lexception_method(struct osic *osic,
                  struct lobject *object,
                  int method, int argc, struct lobject *argv[])
{
#define cast(a) ((struct lexception *)(a))

	switch (method) {
	case LOBJECT_METHOD_GET_ATTR:
		return lexception_get_attr(osic, object, argv[0]);

	case LOBJECT_METHOD_STRING:
		return lexception_string(osic, cast(object));

	case LOBJECT_METHOD_MARK:
		return lexception_mark(osic, cast(object));

	default:
		return lobject_default(osic, object, method, argc, argv);
	}
}

void *
lexception_create(struct osic *osic,
                  struct lobject *message)
{
	struct lexception *self;

	self = lobject_create(osic, sizeof(*self), lexception_method);
	if (self) {
		self->message = message;
	}

	return self;
}

static struct lobject *
lexception_type_method(struct osic *osic,
                       struct lobject *self,
                       int method, int argc, struct lobject *argv[])
{
	switch (method) {
	case LOBJECT_METHOD_CALL: {
		struct lobject *message;
		struct lobject *exception;

		message = NULL;
		if (argc) {
			message = argv[0];
		}
		exception = lexception_create(osic, message);

		return exception;
	}

	case LOBJECT_METHOD_CALLABLE:
		return osic->l_true;

	default:
		return lobject_default(osic, self, method, argc, argv);
	}
}

struct ltype *
lexception_type_create(struct osic *osic)
{
	struct ltype *type;

	type = ltype_create(osic,
	                    "Exception",
	                    lexception_method,
	                    lexception_type_method);
	if (type) {
		osic_add_global(osic, "Exception", type);
	}

	return type;
}
