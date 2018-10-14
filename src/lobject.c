#include "osic.h"
#include "larray.h"
#include "lclass.h"
#include "lstring.h"
#include "linteger.h"
#include "linstance.h"
#include "lexception.h"

#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>

void *
lobject_create(struct osic *osic, size_t size, lobject_method_t method)
{
	struct lobject *self;

	self = osic_allocator_alloc(osic, size);
	if (self) {
		assert(((uintptr_t)self & 0x7) == 0);
		memset(self, 0, size);

		self->l_method = method;
		osic_collector_trace(osic, self);
	}

	return self;
}

int
lobject_destroy(struct osic *osic, struct lobject *self)
{
	if (self && lobject_is_pointer(osic, self)) {
		lobject_method_call(osic,
		                    self,
		                    LOBJECT_METHOD_DESTROY, 0, NULL);
		osic_allocator_free(osic, self);
	}

	return 0;
}

void
lobject_copy(struct osic *osic,
             struct lobject *newobject,
             struct lobject *oldobject,
             size_t size)
{
	if (size > sizeof(struct lobject)) {
		memcpy((char *)newobject + sizeof(struct lobject),
		       (char *)oldobject + sizeof(struct lobject),
		       size - sizeof(struct lobject));
	}
}

struct lobject *
lobject_eq(struct osic *osic, struct lobject *a, struct lobject *b)
{
	return lobject_binop(osic, LOBJECT_METHOD_EQ, a, b);
}

struct lobject *
lobject_unop(struct osic *osic, int method, struct lobject *a)
{
	return lobject_method_call(osic, a, method, 0, NULL);
}

struct lobject *
lobject_binop(struct osic *osic,
              int method, struct lobject *a, struct lobject *b)
{
	struct lobject *argv[1];

	argv[0] = b;
	return lobject_method_call(osic, a, method, 1, argv);
}

struct lobject *
lobject_mark(struct osic *osic, struct lobject *self)
{
	if (lobject_is_pointer(osic, self)) {
		osic_collector_mark(osic, self);
	}

	return NULL;
}

struct lobject *
lobject_length(struct osic *osic, struct lobject *self)
{
	return lobject_method_call(osic,
	                           self,
	                           LOBJECT_METHOD_LENGTH, 0, NULL);
}

struct lobject *
lobject_string(struct osic *osic, struct lobject *self)
{
	return lobject_method_call(osic,
	                           self,
	                           LOBJECT_METHOD_STRING, 0, NULL);
}

struct lobject *
lobject_boolean(struct osic *osic, struct lobject *self)
{
	if (lobject_is_integer(osic, self)) {
		return linteger_method(osic,
	                               self,
	                               LOBJECT_METHOD_BOOLEAN, 0, NULL);
	}
	if (self->l_method == osic->l_boolean_type->method) {
		return self;
	}

	return lobject_method_call(osic,
	                           self,
	                           LOBJECT_METHOD_BOOLEAN, 0, NULL);
}

struct lobject *
lobject_call(struct osic *osic,
             struct lobject *self,
             int argc, struct lobject *argv[])
{
	return lobject_method_call(osic,
	                           self,
	                           LOBJECT_METHOD_CALL, argc, argv);
}

struct lobject *
lobject_all_item(struct osic *osic, struct lobject *self)
{
	struct lobject *value;
	value = lobject_method_call(osic,
	                            self,
	                            LOBJECT_METHOD_ALL_ITEM, 0, NULL);

	return value;
}

struct lobject *
lobject_map_item(struct osic *osic, struct lobject *self)
{
	struct lobject *value;
	value = lobject_method_call(osic,
	                            self,
	                            LOBJECT_METHOD_MAP_ITEM, 0, NULL);

	return value;
}

struct lobject *
lobject_get_item(struct osic *osic,
                 struct lobject *self,
                 struct lobject *name)
{
	struct lobject *value;
	struct lobject *argv[1];

	argv[0] = name;
	value = lobject_method_call(osic,
	                            self,
	                            LOBJECT_METHOD_GET_ITEM, 1, argv);

	return value;
}

struct lobject *
lobject_has_item(struct osic *osic,
                 struct lobject *self,
                 struct lobject *name)
{
	struct lobject *value;
	struct lobject *argv[1];

	argv[0] = name;
	value = lobject_method_call(osic,
	                            self,
	                            LOBJECT_METHOD_HAS_ITEM, 1, argv);

	return value;
}

struct lobject *
lobject_set_item(struct osic *osic,
                 struct lobject *self,
                 struct lobject *name,
                 struct lobject *value)
{
	struct lobject *argv[2];

	argv[0] = name;
	argv[1] = value;
	osic_collector_barrierback(osic, self, name);
	osic_collector_barrierback(osic, self, value);

	return lobject_method_call(osic,
	                           self,
	                           LOBJECT_METHOD_SET_ITEM, 2, argv);
}

struct lobject *
lobject_del_item(struct osic *osic,
                 struct lobject *self,
                 struct lobject *name)
{
	struct lobject *argv[1];

	argv[0] = name;
	return lobject_method_call(osic,
	                           self,
	                           LOBJECT_METHOD_DEL_ITEM, 1, argv);
}

struct lobject *
lobject_add_attr(struct osic *osic,
                 struct lobject *self,
                 struct lobject *name,
                 struct lobject *value)
{
	struct lobject *argv[2];

	argv[0] = name;
	argv[1] = value;
	osic_collector_barrierback(osic, self, name);
	osic_collector_barrierback(osic, self, value);
	return lobject_method_call(osic,
	                           self,
	                           LOBJECT_METHOD_ADD_ATTR, 2, argv);
}

struct lobject *
lobject_get_attr(struct osic *osic,
                 struct lobject *self,
                 struct lobject *name)
{
	struct lobject *value;
	struct lobject *argv[1];

	argv[0] = name;
	value = lobject_method_call(osic,
	                            self,
	                            LOBJECT_METHOD_GET_ATTR, 1, argv);

	return value;
}

struct lobject *
lobject_set_attr(struct osic *osic,
                 struct lobject *self,
                 struct lobject *name,
                 struct lobject *value)
{
	struct lobject *argv[2];

	argv[0] = name;
	argv[1] = value;
	osic_collector_barrierback(osic, self, name);
	osic_collector_barrierback(osic, self, value);
	return lobject_method_call(osic,
	                           self,
	                           LOBJECT_METHOD_SET_ATTR, 2, argv);
}

struct lobject *
lobject_del_attr(struct osic *osic,
                 struct lobject *self,
                 struct lobject *name)
{
	struct lobject *value;
	struct lobject *argv[1];

	argv[0] = name;
	value = lobject_method_call(osic,
	                            self,
	                            LOBJECT_METHOD_DEL_ATTR, 1, argv);

	return value;
}

struct lobject *
lobject_has_attr(struct osic *osic,
                 struct lobject *self,
                 struct lobject *name)
{
	struct lobject *value;
	struct lobject *argv[1];

	argv[0] = name;
	value = lobject_method_call(osic,
	                            self,
	                            LOBJECT_METHOD_HAS_ATTR, 1, argv);

	return value;
}

struct lobject *
lobject_get_slice(struct osic *osic,
                  struct lobject *self,
                  struct lobject *start,
                  struct lobject *stop,
                  struct lobject *step)
{
	struct lobject *argv[3];

	argv[0] = start;
	argv[1] = stop;
	argv[2] = step;
	return lobject_method_call(osic,
	                           self,
	                           LOBJECT_METHOD_GET_SLICE, 3, argv);
}

struct lobject *
lobject_set_slice(struct osic *osic,
                  struct lobject *self,
                  struct lobject *start,
                  struct lobject *stop,
                  struct lobject *step,
                  struct lobject *value)
{
	struct lobject *argv[4];

	argv[0] = start;
	argv[1] = stop;
	argv[2] = step;
	argv[3] = value;
	return lobject_method_call(osic,
	                           self,
	                           LOBJECT_METHOD_SET_SLICE, 4, argv);
}

struct lobject *
lobject_del_slice(struct osic *osic,
                  struct lobject *self,
                  struct lobject *start,
                  struct lobject *stop,
                  struct lobject *step)
{
	struct lobject *argv[3];

	argv[0] = start;
	argv[1] = stop;
	argv[2] = step;
	return lobject_method_call(osic,
	                           self,
	                           LOBJECT_METHOD_DEL_SLICE, 3, argv);
}

struct lobject *
lobject_get_getter(struct osic *osic,
                   struct lobject *self,
                   struct lobject *name)
{
	struct lobject *value;
	struct lobject *argv[1];

	argv[0] = name;
	value = lobject_method_call(osic,
	                            self,
	                            LOBJECT_METHOD_GET_GETTER, 1, argv);

	return value;
}

struct lobject *
lobject_get_setter(struct osic *osic,
                   struct lobject *self,
                   struct lobject *name)
{
	struct lobject *value;
	struct lobject *argv[1];

	argv[0] = name;
	value = lobject_method_call(osic,
	                            self,
	                            LOBJECT_METHOD_GET_SETTER, 1, argv);

	return value;
}

struct lobject *
lobject_call_attr(struct osic *osic,
                  struct lobject *self,
                  struct lobject *name,
                  int argc, struct lobject *argv[])
{
	struct lobject *value;

	value = lobject_get_attr(osic, self, name);
	if (!value) {
		return lobject_error_attribute(osic,
		                               "'%@' has no attribute %@",
		                               self,
		                               name);
	}

	return lobject_call(osic, value, argc, argv);
}

void
lobject_print(struct osic *osic, ...)
{
	va_list ap;

	struct lobject *object;
	struct lobject *string;

	va_start(ap, osic);
	for (;;) {
		object = va_arg(ap, struct lobject *);
		if (!object) {
			break;
		}

		string = lobject_string(osic, object);
		if (string && lobject_is_string(osic, string)) {
			printf("%s ", lstring_to_cstr(osic, string));
		}
	}
	va_end(ap);

	printf("\n");
}

int
lobject_is_integer(struct osic *osic, struct lobject *object)
{
	if ((int)((intptr_t)(object)) & 0x1) {
		return 1;
	}

	return object->l_method == osic->l_integer_type->method;
}

int
lobject_is_pointer(struct osic *osic, struct lobject *object)
{
	return ((int)((intptr_t)(object)) & 0x1) == 0;
}

int
lobject_is_type(struct osic *osic, struct lobject *object)
{
	if (lobject_is_pointer(osic, object)) {
		return object->l_method == osic->l_type_type->method;
	}

	return 0;
}

int
lobject_is_karg(struct osic *osic, struct lobject *object)
{
	if (lobject_is_pointer(osic, object)) {
		return object->l_method == osic->l_karg_type->method;
	}

	return 0;
}

int
lobject_is_varg(struct osic *osic, struct lobject *object)
{
	if (lobject_is_pointer(osic, object)) {
		return object->l_method == osic->l_varg_type->method;
	}

	return 0;
}

int
lobject_is_vkarg(struct osic *osic, struct lobject *object)
{
	if (lobject_is_pointer(osic, object)) {
		return object->l_method == osic->l_vkarg_type->method;
	}

	return 0;
}

int
lobject_is_class(struct osic *osic, struct lobject *object)
{
	if (lobject_is_pointer(osic, object)) {
		return object->l_method == osic->l_class_type->method;
	}

	return 0;
}

int
lobject_is_array(struct osic *osic, struct lobject *object)
{
	if (lobject_is_pointer(osic, object)) {
		return object->l_method == osic->l_array_type->method;
	}

	return 0;
}

int
lobject_is_number(struct osic *osic, struct lobject *object)
{
	if (lobject_is_pointer(osic, object)) {
		return object->l_method == osic->l_number_type->method;
	}

	return 0;
}

int
lobject_is_string(struct osic *osic, struct lobject *object)
{
	if (lobject_is_pointer(osic, object)) {
		return object->l_method == osic->l_string_type->method;
	}

	return 0;
}

int
lobject_is_iterator(struct osic *osic, struct lobject *object)
{
	if (lobject_is_pointer(osic, object)) {
		return object->l_method == osic->l_iterator_type->method;
	}

	return 0;
}

int
lobject_is_instance(struct osic *osic, struct lobject *object)
{
	if (lobject_is_pointer(osic, object)) {
		return object->l_method == osic->l_instance_type->method;
	}

	return 0;
}

int
lobject_is_function(struct osic *osic, struct lobject *object)
{
	if (lobject_is_pointer(osic, object)) {
		return object->l_method == osic->l_function_type->method;
	}

	return 0;
}

int
lobject_is_coroutine(struct osic *osic, struct lobject *object)
{
	if (lobject_is_pointer(osic, object)) {
		return object->l_method == osic->l_coroutine_type->method;
	}

	return 0;
}

int
lobject_is_exception(struct osic *osic, struct lobject *object)
{
	if (lobject_is_pointer(osic, object)) {
		return object->l_method == osic->l_exception_type->method;
	}

	return 0;
}

int
lobject_is_dictionary(struct osic *osic, struct lobject *object)
{
	if (lobject_is_pointer(osic, object)) {
		return object->l_method == osic->l_dictionary_type->method;
	}

	return 0;
}

int
lobject_is_error(struct osic *osic, struct lobject *object)
{
	if (object) {
		if (lobject_is_instance(osic, object)) {
			struct linstance *instance;

			instance = (struct linstance *)object;
			object = instance->native;
			if (!object) {
				return 0;
			}
		}
		if (lobject_is_exception(osic, object)) {
			return ((struct lexception *)object)->throwed;
		}

	}

	return 0;
}

int
lobject_is_equal(struct osic *osic, struct lobject *a, struct lobject *b)
{
	if (a == b) {
		return 1;
	}

	return lobject_eq(osic, a, b) == osic->l_true;
}

struct lobject *
lobject_error_base(struct osic *osic,
                   struct lobject *base,
                   const char *fmt,
                   va_list ap)
{
	const char *p;

	char buffer[4096];
	size_t length;
	struct lobject *value;
	struct lobject *string;
	struct lobject *message;
	struct lobject *exception;

	length = 0;
	memset(buffer, 0, sizeof(buffer));
	for (p = fmt; *p; p++) {
		if (*p == '%') {
			p++;
			switch (*p) {
			case 'd':
				snprintf(buffer + length,
				         sizeof(buffer) - length,
				         "%d",
				         va_arg(ap, int));
				buffer[sizeof(buffer) - 1] = '\0';
				length = strlen(buffer);
				if (length >= sizeof(buffer)) {
					goto out;
				}
				break;
			case '@':
				value = va_arg(ap, struct lobject *);
				string = lobject_string(osic, value);
				snprintf(buffer + length,
				         sizeof(buffer) - length,
				         "%s",
				         lstring_to_cstr(osic, string));
				buffer[sizeof(buffer) - 1] = '\0';
				length = strlen(buffer);
				if (length >= sizeof(buffer)) {
					goto out;
				}
				break;
			case 's':
				snprintf(buffer + length,
				         sizeof(buffer) - length,
				         "%s",
				         va_arg(ap, char *));
				buffer[sizeof(buffer) - 1] = '\0';
				length = strlen(buffer);
				if (length >= sizeof(buffer)) {
					goto out;
				}
				break;
			default:
				buffer[length++] = '%';
				break;
			}
		} else {
			buffer[length++] = *p;
			if (length >= sizeof(buffer)) {
				goto out;
			}
		}
	}

out:
	buffer[sizeof(buffer) - 1] = '\0';
	message = lstring_create(osic, buffer, strlen(buffer));
	exception = lobject_call(osic, base, 1, &message);

	/* return from class's call */
	exception = osic_machine_return_frame(osic, exception);
	exception = lobject_throw(osic, exception);

	return exception;
}

struct lobject *
lobject_error(struct osic *osic,
              struct lobject *base,
              const char *fmt, ...)
{
	va_list ap;
	struct lobject *e;

	va_start(ap, fmt);
	e = lobject_error_base(osic, base, fmt, ap);
	va_end(ap);

	return e;
}

struct lobject *
lobject_error_type(struct osic *osic, const char *fmt, ...)
{
	va_list ap;
	struct lobject *e;

	va_start(ap, fmt);
	e = lobject_error_base(osic, osic->l_type_error, fmt, ap);
	va_end(ap);

	return e;
}

struct lobject *
lobject_error_item(struct osic *osic, const char *fmt, ...)
{
	va_list ap;
	struct lobject *e;

	va_start(ap, fmt);
	e = lobject_error_base(osic, osic->l_item_error, fmt, ap);
	va_end(ap);

	return e;
}

struct lobject *
lobject_error_memory(struct osic *osic, const char *fmt, ...)
{
	va_list ap;
	struct lobject *e;

	va_start(ap, fmt);
	e = lobject_error_base(osic, osic->l_memory_error, fmt, ap);
	va_end(ap);

	return e;
}

struct lobject *
lobject_error_runtime(struct osic *osic, const char *fmt, ...)
{
	va_list ap;
	struct lobject *e;

	va_start(ap, fmt);
	e = lobject_error_base(osic, osic->l_runtime_error, fmt, ap);
	va_end(ap);

	return e;
}

struct lobject *
lobject_error_argument(struct osic *osic, const char *fmt, ...)
{
	va_list ap;
	struct lobject *e;

	va_start(ap, fmt);
	e = lobject_error_base(osic, osic->l_argument_error, fmt, ap);
	va_end(ap);

	return e;
}

struct lobject *
lobject_error_attribute(struct osic *osic, const char *fmt, ...)
{
	va_list ap;
	struct lobject *e;

	va_start(ap, fmt);
	e = lobject_error_base(osic, osic->l_attribute_error, fmt, ap);
	va_end(ap);

	return e;
}

struct lobject *
lobject_error_arithmetic(struct osic *osic, const char *fmt, ...)
{
	va_list ap;
	struct lobject *e;

	va_start(ap, fmt);
	e = lobject_error_base(osic, osic->l_arithmetic_error, fmt, ap);
	va_end(ap);

	return e;
}

void *
lobject_error_not_callable(struct osic *osic, struct lobject *object)
{
	return lobject_error(osic,
	                     osic->l_not_callable_error,
	                     "'%@' is not callable",
	                     object);
}

void *
lobject_error_not_iterable(struct osic *osic, struct lobject *object)
{
	return lobject_error(osic,
	                     osic->l_not_callable_error,
	                     "'%@' is not iterable",
	                     object);
}

void *
lobject_error_not_implemented(struct osic *osic)
{
	return lobject_error(osic, osic->l_not_implemented_error, "");
}

struct lobject *
lobject_throw(struct osic *osic, struct lobject *error)
{
	struct lobject *clazz;
	struct lobject *message;
	struct linstance *instance;
	struct lexception *exception;

	clazz = NULL;
	exception = NULL;
	if (lobject_is_exception(osic, error)) {
		exception = (struct lexception *)error;
	} else if (lobject_is_instance(osic, error)) {
		instance = (struct linstance *)error;

		if (instance->native &&
		    lobject_is_exception(osic, instance->native))
		{
			clazz = (struct lobject *)instance->clazz;
			exception = (struct lexception *)instance->native;
		}
	}

	if (!exception) {
		message = lstring_create(osic, "throw non-Exception", 19);
		error = lobject_call(osic, osic->l_type_error, 1, &message);
		instance = (struct linstance *)error;
		clazz = (struct lobject *)instance->clazz;
		exception = (struct lexception *)instance->native;
	}

	assert(exception);

	exception->clazz = clazz;
	exception->throwed = 1;

	return error;
}

struct lobject *
lobject_method_call(struct osic *osic,
                    struct lobject *self,
                    int method, int argc, struct lobject *argv[])
{
	if (lobject_is_pointer(osic, self) && self->l_method) {
		return self->l_method(osic, self, method, argc, argv);
	}

	if (lobject_is_integer(osic, self)) {
		return linteger_method(osic, self, method, argc, argv);
	}

	return lobject_default(osic, self, method, argc, argv);
}

struct lobject *
lobject_default(struct osic *osic,
                struct lobject *self,
                int method, int argc, struct lobject *argv[])
{
	switch (method) {
	case LOBJECT_METHOD_EQ:
		if (self == argv[0]) {
			return osic->l_true;
		}
		return osic->l_false;

	case LOBJECT_METHOD_NE:
		if (!lobject_is_equal(osic, self, argv[0])) {
			return osic->l_true;
		}
		return osic->l_false;

	case LOBJECT_METHOD_LE:
		return lobject_eq(osic, self, argv[0]);

	case LOBJECT_METHOD_GE:
		return lobject_eq(osic, self, argv[0]);

	case LOBJECT_METHOD_BOOLEAN:
		return osic->l_true;

	case LOBJECT_METHOD_CALL:
		return lobject_error_not_callable(osic, self);

	case LOBJECT_METHOD_CALLABLE:
		return osic->l_false;

	case LOBJECT_METHOD_GET_ITEM:
		return lobject_error_not_implemented(osic);

	case LOBJECT_METHOD_SET_ITEM:
		return lobject_error_not_implemented(osic);

	case LOBJECT_METHOD_GET_ATTR:
		return NULL;

	case LOBJECT_METHOD_SET_ATTR:
		return lobject_error_not_implemented(osic);

	case LOBJECT_METHOD_GET_GETTER:
		return NULL;

	case LOBJECT_METHOD_GET_SETTER:
		return NULL;

	case LOBJECT_METHOD_SUPER:
		return NULL;

	case LOBJECT_METHOD_SUBCLASS:
		return NULL;

	case LOBJECT_METHOD_INSTANCE:
		return NULL;

	case LOBJECT_METHOD_STRING: {
		char buffer[32];
		snprintf(buffer, sizeof(buffer), "<object %p>", (void *)self);
		return lstring_create(osic, buffer, strlen(buffer));
	}

	case LOBJECT_METHOD_HASH:
		return linteger_create_from_long(osic, (long)self);

	case LOBJECT_METHOD_MARK:
		return NULL;

	case LOBJECT_METHOD_DESTROY:
		return NULL;

	default:
		return lobject_error_type(osic,
		                          "unsupport method for %@",
		                          self);
	}
}

struct lobject *
lobject_get_default_subclassof(struct osic *osic,
                            struct lobject *self,
                            int argc, struct lobject *argv[])
{
	int i;
	struct lclass *clazz;
	struct lobject *item;

	if (lobject_is_class(osic, self)) {
		clazz = (struct lclass *)self;
		for (i = 0; i < larray_length(osic, clazz->bases); i++) {
			item = larray_get_item(osic, clazz->bases, i);
			if (lobject_is_equal(osic, item, argv[0])) {
				return osic->l_true;
			}
		}
	}

	return osic->l_false;
}

struct lobject *
lobject_get_default_instanceof(struct osic *osic,
                           struct lobject *self,
                           int argc, struct lobject *argv[])
{
	int i;

	if (lobject_is_instance(osic, self)) {
		struct lobject *clazz;

		clazz = (struct lobject *)((struct linstance *)self)->clazz;
		if (!clazz) {
			return osic->l_false;
		}
		for (i = 0; i < argc; i++) {
			if (lobject_is_equal(osic, clazz, argv[i])) {
				return osic->l_true;
			}
		}

		return lobject_get_default_subclassof(osic, clazz, argc, argv);
	}

	if (lobject_is_pointer(osic, self)) {
		for (i = 0; i < argc; i++) {
			struct ltype *type;

			if (lobject_is_type(osic, argv[i])) {
				type = (struct ltype *)argv[i];
				if (self->l_method == type->method) {
					return osic->l_true;
				}
			}
		}
		return osic->l_false;
	}

	if (lobject_is_integer(osic, self)) {
		for (i = 0; i < argc; i++) {
			if (osic->l_integer_type == (struct ltype *)argv[i]) {
				return osic->l_true;
			}
		}
		return osic->l_false;
	}

	return osic->l_false;
}

struct lobject *
lobject_get_default_callable(struct osic *osic,
                         struct lobject *self,
                         int argc, struct lobject *argv[])
{
	return self->l_method(osic, self, LOBJECT_METHOD_CALLABLE, 0, NULL);
}

struct lobject *
lobject_get_default_string(struct osic *osic,
                       struct lobject *self,
                       int argc, struct lobject *argv[])
{
	return lobject_string(osic, self);
}

struct lobject *
lobject_get_default_length(struct osic *osic,
                        struct lobject *self,
                        int argc, struct lobject *argv[])
{
	return lobject_length(osic, self);
}

struct lobject *
lobject_get_default_get_attr(struct osic *osic,
                         struct lobject *self,
                         int argc, struct lobject *argv[])
{
	return lobject_get_attr(osic, self, argv[0]);
}

struct lobject *
lobject_get_default_set_attr(struct osic *osic,
                         struct lobject *self,
                         int argc, struct lobject *argv[])
{
	return lobject_set_attr(osic, self, argv[0], argv[1]);
}

struct lobject *
lobject_get_default_del_attr(struct osic *osic,
                             struct lobject *self,
                             int argc, struct lobject *argv[])
{
	return lobject_del_attr(osic, self, argv[0]);
}

struct lobject *
lobject_get_default_has_attr(struct osic *osic,
                         struct lobject *self,
                         int argc, struct lobject *argv[])
{
	return lobject_has_attr(osic, self, argv[0]);
}

struct lobject *
lobject_default_get_attr(struct osic *osic,
                         struct lobject *self,
                         struct lobject *name)
{
	const char *cstr;
	struct lobject *value;

	if (!lobject_is_string(osic, name)) {
		return NULL;
	}

	value = lobject_get_attr(osic, self, name);
	if (value) {
		return value;
	}

	cstr = lstring_to_cstr(osic, name);
	if (strcmp(cstr, "__string__") == 0) {
		return lfunction_create(osic,
		                        name,
		                        self,
		                        lobject_get_default_string);
	}

	if (strcmp(cstr, "__length__") == 0) {
		return lfunction_create(osic,
		                        name,
		                        self,
		                        lobject_get_default_length);
	}

	if (strcmp(cstr, "__callable__") == 0) {
		return lfunction_create(osic,
		                        name,
		                        self,
		                        lobject_get_default_callable);
	}

	if (strcmp(cstr, "__instanceof__") == 0) {
		return lfunction_create(osic,
		                        name,
		                        self,
		                        lobject_get_default_instanceof);
	}

	if (strcmp(cstr, "__subclassof__") == 0) {
		return lfunction_create(osic,
		                        name,
		                        self,
		                        lobject_get_default_subclassof);
	}

	if (strcmp(cstr, "__get_attr__") == 0) {
		return lfunction_create(osic,
		                        name,
		                        self,
		                        lobject_get_default_get_attr);
	}

	if (strcmp(cstr, "__set_attr__") == 0) {
		return lfunction_create(osic,
		                        name,
		                        self,
		                        lobject_get_default_set_attr);
	}

	if (strcmp(cstr, "__del_attr__") == 0) {
		return lfunction_create(osic,
		                        name,
		                        self,
		                        lobject_get_default_del_attr);
	}

	if (strcmp(cstr, "__has_attr__") == 0) {
		return lfunction_create(osic,
		                        name,
		                        self,
		                        lobject_get_default_has_attr);
	}

	return NULL;
}
