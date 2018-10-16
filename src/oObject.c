#include "osic.h"
#include "oArray.h"
#include "oClass.h"
#include "oString.h"
#include "oInteger.h"
#include "oInstance.h"
#include "oException.h"

#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>

void *
oobject_create(struct osic *osic, size_t size, oobject_method_t method)
{
	struct oobject *self;

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
oobject_destroy(struct osic *osic, struct oobject *self)
{
	if (self && oobject_is_pointer(osic, self)) {
		oobject_method_call(osic,
		                    self,
		                    OOBJECT_METHOD_DESTROY, 0, NULL);
		osic_allocator_free(osic, self);
	}

	return 0;
}

void
oobject_copy(struct osic *osic,
             struct oobject *newobject,
             struct oobject *oldobject,
             size_t size)
{
	if (size > sizeof(struct oobject)) {
		memcpy((char *)newobject + sizeof(struct oobject),
		       (char *)oldobject + sizeof(struct oobject),
		       size - sizeof(struct oobject));
	}
}

struct oobject *
oobject_eq(struct osic *osic, struct oobject *a, struct oobject *b)
{
	return oobject_binop(osic, OOBJECT_METHOD_EQ, a, b);
}

struct oobject *
oobject_unop(struct osic *osic, int method, struct oobject *a)
{
	return oobject_method_call(osic, a, method, 0, NULL);
}

struct oobject *
oobject_binop(struct osic *osic,
              int method, struct oobject *a, struct oobject *b)
{
	struct oobject *argv[1];

	argv[0] = b;
	return oobject_method_call(osic, a, method, 1, argv);
}

struct oobject *
oobject_mark(struct osic *osic, struct oobject *self)
{
	if (oobject_is_pointer(osic, self)) {
		osic_collector_mark(osic, self);
	}

	return NULL;
}

struct oobject *
oobject_length(struct osic *osic, struct oobject *self)
{
	return oobject_method_call(osic,
	                           self,
	                           OOBJECT_METHOD_LENGTH, 0, NULL);
}

struct oobject *
oobject_string(struct osic *osic, struct oobject *self)
{
	return oobject_method_call(osic,
	                           self,
	                           OOBJECT_METHOD_STRING, 0, NULL);
}

struct oobject *
oobject_boolean(struct osic *osic, struct oobject *self)
{
	if (oobject_is_integer(osic, self)) {
		return ointeger_method(osic,
	                               self,
	                               OOBJECT_METHOD_BOOLEAN, 0, NULL);
	}
	if (self->l_method == osic->l_boolean_type->method) {
		return self;
	}

	return oobject_method_call(osic,
	                           self,
	                           OOBJECT_METHOD_BOOLEAN, 0, NULL);
}

struct oobject *
oobject_call(struct osic *osic,
             struct oobject *self,
             int argc, struct oobject *argv[])
{
	return oobject_method_call(osic,
	                           self,
	                           OOBJECT_METHOD_CALL, argc, argv);
}

struct oobject *
oobject_all_item(struct osic *osic, struct oobject *self)
{
	struct oobject *value;
	value = oobject_method_call(osic,
	                            self,
	                            OOBJECT_METHOD_ALL_ITEM, 0, NULL);

	return value;
}

struct oobject *
oobject_map_item(struct osic *osic, struct oobject *self)
{
	struct oobject *value;
	value = oobject_method_call(osic,
	                            self,
	                            OOBJECT_METHOD_MAP_ITEM, 0, NULL);

	return value;
}

struct oobject *
oobject_get_item(struct osic *osic,
                 struct oobject *self,
                 struct oobject *name)
{
	struct oobject *value;
	struct oobject *argv[1];

	argv[0] = name;
	value = oobject_method_call(osic,
	                            self,
	                            OOBJECT_METHOD_GET_ITEM, 1, argv);

	return value;
}

struct oobject *
oobject_has_item(struct osic *osic,
                 struct oobject *self,
                 struct oobject *name)
{
	struct oobject *value;
	struct oobject *argv[1];

	argv[0] = name;
	value = oobject_method_call(osic,
	                            self,
	                            OOBJECT_METHOD_HAS_ITEM, 1, argv);

	return value;
}

struct oobject *
oobject_set_item(struct osic *osic,
                 struct oobject *self,
                 struct oobject *name,
                 struct oobject *value)
{
	struct oobject *argv[2];

	argv[0] = name;
	argv[1] = value;
	osic_collector_barrierback(osic, self, name);
	osic_collector_barrierback(osic, self, value);

	return oobject_method_call(osic,
	                           self,
	                           OOBJECT_METHOD_SET_ITEM, 2, argv);
}

struct oobject *
oobject_del_item(struct osic *osic,
                 struct oobject *self,
                 struct oobject *name)
{
	struct oobject *argv[1];

	argv[0] = name;
	return oobject_method_call(osic,
	                           self,
	                           OOBJECT_METHOD_DEL_ITEM, 1, argv);
}

struct oobject *
oobject_add_attr(struct osic *osic,
                 struct oobject *self,
                 struct oobject *name,
                 struct oobject *value)
{
	struct oobject *argv[2];

	argv[0] = name;
	argv[1] = value;
	osic_collector_barrierback(osic, self, name);
	osic_collector_barrierback(osic, self, value);
	return oobject_method_call(osic,
	                           self,
	                           OOBJECT_METHOD_ADD_ATTR, 2, argv);
}

struct oobject *
oobject_get_attr(struct osic *osic,
                 struct oobject *self,
                 struct oobject *name)
{
	struct oobject *value;
	struct oobject *argv[1];

	argv[0] = name;
	value = oobject_method_call(osic,
	                            self,
	                            OOBJECT_METHOD_GET_ATTR, 1, argv);

	return value;
}

struct oobject *
oobject_set_attr(struct osic *osic,
                 struct oobject *self,
                 struct oobject *name,
                 struct oobject *value)
{
	struct oobject *argv[2];

	argv[0] = name;
	argv[1] = value;
	osic_collector_barrierback(osic, self, name);
	osic_collector_barrierback(osic, self, value);
	return oobject_method_call(osic,
	                           self,
	                           OOBJECT_METHOD_SET_ATTR, 2, argv);
}

struct oobject *
oobject_del_attr(struct osic *osic,
                 struct oobject *self,
                 struct oobject *name)
{
	struct oobject *value;
	struct oobject *argv[1];

	argv[0] = name;
	value = oobject_method_call(osic,
	                            self,
	                            OOBJECT_METHOD_DEL_ATTR, 1, argv);

	return value;
}

struct oobject *
oobject_has_attr(struct osic *osic,
                 struct oobject *self,
                 struct oobject *name)
{
	struct oobject *value;
	struct oobject *argv[1];

	argv[0] = name;
	value = oobject_method_call(osic,
	                            self,
	                            OOBJECT_METHOD_HAS_ATTR, 1, argv);

	return value;
}

struct oobject *
oobject_get_slice(struct osic *osic,
                  struct oobject *self,
                  struct oobject *start,
                  struct oobject *stop,
                  struct oobject *step)
{
	struct oobject *argv[3];

	argv[0] = start;
	argv[1] = stop;
	argv[2] = step;
	return oobject_method_call(osic,
	                           self,
	                           OOBJECT_METHOD_GET_SLICE, 3, argv);
}

struct oobject *
oobject_set_slice(struct osic *osic,
                  struct oobject *self,
                  struct oobject *start,
                  struct oobject *stop,
                  struct oobject *step,
                  struct oobject *value)
{
	struct oobject *argv[4];

	argv[0] = start;
	argv[1] = stop;
	argv[2] = step;
	argv[3] = value;
	return oobject_method_call(osic,
	                           self,
	                           OOBJECT_METHOD_SET_SLICE, 4, argv);
}

struct oobject *
oobject_del_slice(struct osic *osic,
                  struct oobject *self,
                  struct oobject *start,
                  struct oobject *stop,
                  struct oobject *step)
{
	struct oobject *argv[3];

	argv[0] = start;
	argv[1] = stop;
	argv[2] = step;
	return oobject_method_call(osic,
	                           self,
	                           OOBJECT_METHOD_DEL_SLICE, 3, argv);
}

struct oobject *
oobject_get_getter(struct osic *osic,
                   struct oobject *self,
                   struct oobject *name)
{
	struct oobject *value;
	struct oobject *argv[1];

	argv[0] = name;
	value = oobject_method_call(osic,
	                            self,
	                            OOBJECT_METHOD_GET_GETTER, 1, argv);

	return value;
}

struct oobject *
oobject_get_setter(struct osic *osic,
                   struct oobject *self,
                   struct oobject *name)
{
	struct oobject *value;
	struct oobject *argv[1];

	argv[0] = name;
	value = oobject_method_call(osic,
	                            self,
	                            OOBJECT_METHOD_GET_SETTER, 1, argv);

	return value;
}

struct oobject *
oobject_call_attr(struct osic *osic,
                  struct oobject *self,
                  struct oobject *name,
                  int argc, struct oobject *argv[])
{
	struct oobject *value;

	value = oobject_get_attr(osic, self, name);
	if (!value) {
		return oobject_error_attribute(osic,
		                               "'%@' has no attribute %@",
		                               self,
		                               name);
	}

	return oobject_call(osic, value, argc, argv);
}

void
oobject_print(struct osic *osic, ...)
{
	va_list ap;

	struct oobject *object;
	struct oobject *string;

	va_start(ap, osic);
	for (;;) {
		object = va_arg(ap, struct oobject *);
		if (!object) {
			break;
		}

		string = oobject_string(osic, object);
		if (string && oobject_is_string(osic, string)) {
			printf("%s ", ostring_to_cstr(osic, string));
		}
	}
	va_end(ap);

	printf("\n");
}

int
oobject_is_integer(struct osic *osic, struct oobject *object)
{
	if ((int)((intptr_t)(object)) & 0x1) {
		return 1;
	}

	return object->l_method == osic->l_integer_type->method;
}

int
oobject_is_pointer(struct osic *osic, struct oobject *object)
{
	return ((int)((intptr_t)(object)) & 0x1) == 0;
}

int
oobject_is_type(struct osic *osic, struct oobject *object)
{
	if (oobject_is_pointer(osic, object)) {
		return object->l_method == osic->l_type_type->method;
	}

	return 0;
}

int
oobject_is_karg(struct osic *osic, struct oobject *object)
{
	if (oobject_is_pointer(osic, object)) {
		return object->l_method == osic->l_karg_type->method;
	}

	return 0;
}

int
oobject_is_varg(struct osic *osic, struct oobject *object)
{
	if (oobject_is_pointer(osic, object)) {
		return object->l_method == osic->l_varg_type->method;
	}

	return 0;
}

int
oobject_is_vkarg(struct osic *osic, struct oobject *object)
{
	if (oobject_is_pointer(osic, object)) {
		return object->l_method == osic->l_vkarg_type->method;
	}

	return 0;
}

int
oobject_is_class(struct osic *osic, struct oobject *object)
{
	if (oobject_is_pointer(osic, object)) {
		return object->l_method == osic->l_class_type->method;
	}

	return 0;
}

int
oobject_is_array(struct osic *osic, struct oobject *object)
{
	if (oobject_is_pointer(osic, object)) {
		return object->l_method == osic->l_array_type->method;
	}

	return 0;
}

int
oobject_is_number(struct osic *osic, struct oobject *object)
{
	if (oobject_is_pointer(osic, object)) {
		return object->l_method == osic->l_number_type->method;
	}

	return 0;
}

int
oobject_is_string(struct osic *osic, struct oobject *object)
{
	if (oobject_is_pointer(osic, object)) {
		return object->l_method == osic->l_string_type->method;
	}

	return 0;
}

int
oobject_is_iterator(struct osic *osic, struct oobject *object)
{
	if (oobject_is_pointer(osic, object)) {
		return object->l_method == osic->l_iterator_type->method;
	}

	return 0;
}

int
oobject_is_instance(struct osic *osic, struct oobject *object)
{
	if (oobject_is_pointer(osic, object)) {
		return object->l_method == osic->l_instance_type->method;
	}

	return 0;
}

int
oobject_is_function(struct osic *osic, struct oobject *object)
{
	if (oobject_is_pointer(osic, object)) {
		return object->l_method == osic->l_function_type->method;
	}

	return 0;
}

int
oobject_is_coroutine(struct osic *osic, struct oobject *object)
{
	if (oobject_is_pointer(osic, object)) {
		return object->l_method == osic->l_coroutine_type->method;
	}

	return 0;
}

int
oobject_is_exception(struct osic *osic, struct oobject *object)
{
	if (oobject_is_pointer(osic, object)) {
		return object->l_method == osic->l_exception_type->method;
	}

	return 0;
}

int
oobject_is_dictionary(struct osic *osic, struct oobject *object)
{
	if (oobject_is_pointer(osic, object)) {
		return object->l_method == osic->l_dictionary_type->method;
	}

	return 0;
}

int
oobject_is_error(struct osic *osic, struct oobject *object)
{
	if (object) {
		if (oobject_is_instance(osic, object)) {
			struct oinstance *instance;

			instance = (struct oinstance *)object;
			object = instance->native;
			if (!object) {
				return 0;
			}
		}
		if (oobject_is_exception(osic, object)) {
			return ((struct oexception *)object)->throwed;
		}

	}

	return 0;
}

int
oobject_is_equal(struct osic *osic, struct oobject *a, struct oobject *b)
{
	if (a == b) {
		return 1;
	}

	return oobject_eq(osic, a, b) == osic->l_true;
}

struct oobject *
oobject_error_base(struct osic *osic,
                   struct oobject *base,
                   const char *fmt,
                   va_list ap)
{
	const char *p;

	char buffer[4096];
	size_t length;
	struct oobject *value;
	struct oobject *string;
	struct oobject *message;
	struct oobject *exception;

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
				value = va_arg(ap, struct oobject *);
				string = oobject_string(osic, value);
				snprintf(buffer + length,
				         sizeof(buffer) - length,
				         "%s",
				         ostring_to_cstr(osic, string));
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
	message = ostring_create(osic, buffer, strlen(buffer));
	exception = oobject_call(osic, base, 1, &message);

	/* return from class's call */
	exception = osic_machine_return_frame(osic, exception);
	exception = oobject_throw(osic, exception);

	return exception;
}

struct oobject *
oobject_error(struct osic *osic,
              struct oobject *base,
              const char *fmt, ...)
{
	va_list ap;
	struct oobject *e;

	va_start(ap, fmt);
	e = oobject_error_base(osic, base, fmt, ap);
	va_end(ap);

	return e;
}

struct oobject *
oobject_error_type(struct osic *osic, const char *fmt, ...)
{
	va_list ap;
	struct oobject *e;

	va_start(ap, fmt);
	e = oobject_error_base(osic, osic->l_type_error, fmt, ap);
	va_end(ap);

	return e;
}

struct oobject *
oobject_error_item(struct osic *osic, const char *fmt, ...)
{
	va_list ap;
	struct oobject *e;

	va_start(ap, fmt);
	e = oobject_error_base(osic, osic->l_item_error, fmt, ap);
	va_end(ap);

	return e;
}

struct oobject *
oobject_error_memory(struct osic *osic, const char *fmt, ...)
{
	va_list ap;
	struct oobject *e;

	va_start(ap, fmt);
	e = oobject_error_base(osic, osic->l_memory_error, fmt, ap);
	va_end(ap);

	return e;
}

struct oobject *
oobject_error_runtime(struct osic *osic, const char *fmt, ...)
{
	va_list ap;
	struct oobject *e;

	va_start(ap, fmt);
	e = oobject_error_base(osic, osic->l_runtime_error, fmt, ap);
	va_end(ap);

	return e;
}

struct oobject *
oobject_error_argument(struct osic *osic, const char *fmt, ...)
{
	va_list ap;
	struct oobject *e;

	va_start(ap, fmt);
	e = oobject_error_base(osic, osic->l_argument_error, fmt, ap);
	va_end(ap);

	return e;
}

struct oobject *
oobject_error_attribute(struct osic *osic, const char *fmt, ...)
{
	va_list ap;
	struct oobject *e;

	va_start(ap, fmt);
	e = oobject_error_base(osic, osic->l_attribute_error, fmt, ap);
	va_end(ap);

	return e;
}

struct oobject *
oobject_error_arithmetic(struct osic *osic, const char *fmt, ...)
{
	va_list ap;
	struct oobject *e;

	va_start(ap, fmt);
	e = oobject_error_base(osic, osic->l_arithmetic_error, fmt, ap);
	va_end(ap);

	return e;
}

void *
oobject_error_not_callable(struct osic *osic, struct oobject *object)
{
	return oobject_error(osic,
	                     osic->l_not_callable_error,
	                     "'%@' is not callable",
	                     object);
}

void *
oobject_error_not_iterable(struct osic *osic, struct oobject *object)
{
	return oobject_error(osic,
	                     osic->l_not_callable_error,
	                     "'%@' is not iterable",
	                     object);
}

void *
oobject_error_not_implemented(struct osic *osic)
{
	return oobject_error(osic, osic->l_not_implemented_error, "");
}

struct oobject *
oobject_throw(struct osic *osic, struct oobject *error)
{
	struct oobject *clazz;
	struct oobject *message;
	struct oinstance *instance;
	struct oexception *exception;

	clazz = NULL;
	exception = NULL;
	if (oobject_is_exception(osic, error)) {
		exception = (struct oexception *)error;
	} else if (oobject_is_instance(osic, error)) {
		instance = (struct oinstance *)error;

		if (instance->native &&
		    oobject_is_exception(osic, instance->native))
		{
			clazz = (struct oobject *)instance->clazz;
			exception = (struct oexception *)instance->native;
		}
	}

	if (!exception) {
		message = ostring_create(osic, "throw non-Exception", 19);
		error = oobject_call(osic, osic->l_type_error, 1, &message);
		instance = (struct oinstance *)error;
		clazz = (struct oobject *)instance->clazz;
		exception = (struct oexception *)instance->native;
	}

	assert(exception);

	exception->clazz = clazz;
	exception->throwed = 1;

	return error;
}

struct oobject *
oobject_method_call(struct osic *osic,
                    struct oobject *self,
                    int method, int argc, struct oobject *argv[])
{
	if (oobject_is_pointer(osic, self) && self->l_method) {
		return self->l_method(osic, self, method, argc, argv);
	}

	if (oobject_is_integer(osic, self)) {
		return ointeger_method(osic, self, method, argc, argv);
	}

	return oobject_default(osic, self, method, argc, argv);
}

struct oobject *
oobject_default(struct osic *osic,
                struct oobject *self,
                int method, int argc, struct oobject *argv[])
{
	switch (method) {
	case OOBJECT_METHOD_EQ:
		if (self == argv[0]) {
			return osic->l_true;
		}
		return osic->l_false;

	case OOBJECT_METHOD_NE:
		if (!oobject_is_equal(osic, self, argv[0])) {
			return osic->l_true;
		}
		return osic->l_false;

	case OOBJECT_METHOD_LE:
		return oobject_eq(osic, self, argv[0]);

	case OOBJECT_METHOD_GE:
		return oobject_eq(osic, self, argv[0]);

	case OOBJECT_METHOD_BOOLEAN:
		return osic->l_true;

	case OOBJECT_METHOD_CALL:
		return oobject_error_not_callable(osic, self);

	case OOBJECT_METHOD_CALLABLE:
		return osic->l_false;

	case OOBJECT_METHOD_GET_ITEM:
		return oobject_error_not_implemented(osic);

	case OOBJECT_METHOD_SET_ITEM:
		return oobject_error_not_implemented(osic);

	case OOBJECT_METHOD_GET_ATTR:
		return NULL;

	case OOBJECT_METHOD_SET_ATTR:
		return oobject_error_not_implemented(osic);

	case OOBJECT_METHOD_GET_GETTER:
		return NULL;

	case OOBJECT_METHOD_GET_SETTER:
		return NULL;

	case OOBJECT_METHOD_SUPER:
		return NULL;

	case OOBJECT_METHOD_SUBCLASS:
		return NULL;

	case OOBJECT_METHOD_INSTANCE:
		return NULL;

	case OOBJECT_METHOD_STRING: {
		char buffer[32];
		snprintf(buffer, sizeof(buffer), "<object %p>", (void *)self);
		return ostring_create(osic, buffer, strlen(buffer));
	}

	case OOBJECT_METHOD_HASH:
		return ointeger_create_from_long(osic, (long)self);

	case OOBJECT_METHOD_MARK:
		return NULL;

	case OOBJECT_METHOD_DESTROY:
		return NULL;

	default:
		return oobject_error_type(osic,
		                          "unsupport method for %@",
		                          self);
	}
}

struct oobject *
oobject_get_default_subclassof(struct osic *osic,
                            struct oobject *self,
                            int argc, struct oobject *argv[])
{
	int i;
	struct oclass *clazz;
	struct oobject *item;

	if (oobject_is_class(osic, self)) {
		clazz = (struct oclass *)self;
		for (i = 0; i < oarray_length(osic, clazz->bases); i++) {
			item = oarray_get_item(osic, clazz->bases, i);
			if (oobject_is_equal(osic, item, argv[0])) {
				return osic->l_true;
			}
		}
	}

	return osic->l_false;
}

struct oobject *
oobject_get_default_instanceof(struct osic *osic,
                           struct oobject *self,
                           int argc, struct oobject *argv[])
{
	int i;

	if (oobject_is_instance(osic, self)) {
		struct oobject *clazz;

		clazz = (struct oobject *)((struct oinstance *)self)->clazz;
		if (!clazz) {
			return osic->l_false;
		}
		for (i = 0; i < argc; i++) {
			if (oobject_is_equal(osic, clazz, argv[i])) {
				return osic->l_true;
			}
		}

		return oobject_get_default_subclassof(osic, clazz, argc, argv);
	}

	if (oobject_is_pointer(osic, self)) {
		for (i = 0; i < argc; i++) {
			struct otype *type;

			if (oobject_is_type(osic, argv[i])) {
				type = (struct otype *)argv[i];
				if (self->l_method == type->method) {
					return osic->l_true;
				}
			}
		}
		return osic->l_false;
	}

	if (oobject_is_integer(osic, self)) {
		for (i = 0; i < argc; i++) {
			if (osic->l_integer_type == (struct otype *)argv[i]) {
				return osic->l_true;
			}
		}
		return osic->l_false;
	}

	return osic->l_false;
}

struct oobject *
oobject_get_default_callable(struct osic *osic,
                         struct oobject *self,
                         int argc, struct oobject *argv[])
{
	return self->l_method(osic, self, OOBJECT_METHOD_CALLABLE, 0, NULL);
}

struct oobject *
oobject_get_default_string(struct osic *osic,
                       struct oobject *self,
                       int argc, struct oobject *argv[])
{
	return oobject_string(osic, self);
}

struct oobject *
oobject_get_default_length(struct osic *osic,
                        struct oobject *self,
                        int argc, struct oobject *argv[])
{
	return oobject_length(osic, self);
}

struct oobject *
oobject_get_default_get_attr(struct osic *osic,
                         struct oobject *self,
                         int argc, struct oobject *argv[])
{
	return oobject_get_attr(osic, self, argv[0]);
}

struct oobject *
oobject_get_default_set_attr(struct osic *osic,
                         struct oobject *self,
                         int argc, struct oobject *argv[])
{
	return oobject_set_attr(osic, self, argv[0], argv[1]);
}

struct oobject *
oobject_get_default_del_attr(struct osic *osic,
                             struct oobject *self,
                             int argc, struct oobject *argv[])
{
	return oobject_del_attr(osic, self, argv[0]);
}

struct oobject *
oobject_get_default_has_attr(struct osic *osic,
                         struct oobject *self,
                         int argc, struct oobject *argv[])
{
	return oobject_has_attr(osic, self, argv[0]);
}

struct oobject *
oobject_default_get_attr(struct osic *osic,
                         struct oobject *self,
                         struct oobject *name)
{
	const char *cstr;
	struct oobject *value;

	if (!oobject_is_string(osic, name)) {
		return NULL;
	}

	value = oobject_get_attr(osic, self, name);
	if (value) {
		return value;
	}

	cstr = ostring_to_cstr(osic, name);
	if (strcmp(cstr, "__string__") == 0) {
		return ofunction_create(osic,
		                        name,
		                        self,
		                        oobject_get_default_string);
	}

	if (strcmp(cstr, "__length__") == 0) {
		return ofunction_create(osic,
		                        name,
		                        self,
		                        oobject_get_default_length);
	}

	if (strcmp(cstr, "__callable__") == 0) {
		return ofunction_create(osic,
		                        name,
		                        self,
		                        oobject_get_default_callable);
	}

	if (strcmp(cstr, "__instanceof__") == 0) {
		return ofunction_create(osic,
		                        name,
		                        self,
		                        oobject_get_default_instanceof);
	}

	if (strcmp(cstr, "__subclassof__") == 0) {
		return ofunction_create(osic,
		                        name,
		                        self,
		                        oobject_get_default_subclassof);
	}

	if (strcmp(cstr, "__get_attr__") == 0) {
		return ofunction_create(osic,
		                        name,
		                        self,
		                        oobject_get_default_get_attr);
	}

	if (strcmp(cstr, "__set_attr__") == 0) {
		return ofunction_create(osic,
		                        name,
		                        self,
		                        oobject_get_default_set_attr);
	}

	if (strcmp(cstr, "__del_attr__") == 0) {
		return ofunction_create(osic,
		                        name,
		                        self,
		                        oobject_get_default_del_attr);
	}

	if (strcmp(cstr, "__has_attr__") == 0) {
		return ofunction_create(osic,
		                        name,
		                        self,
		                        oobject_get_default_has_attr);
	}

	return NULL;
}
