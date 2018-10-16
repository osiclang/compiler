#ifndef OSIC_OOBJECT_H
#define OSIC_OOBJECT_H

struct osic;
struct otype;

typedef struct oobject *(*oobject_method_t)(struct osic *,
                                            struct oobject *,
                                            int,
                                            int,
                                            struct oobject *[]);

enum {
	OOBJECT_METHOD_ADD,
	OOBJECT_METHOD_SUB,
	OOBJECT_METHOD_MUL,
	OOBJECT_METHOD_DIV,
	OOBJECT_METHOD_MOD,
	OOBJECT_METHOD_POS,
	OOBJECT_METHOD_NEG,
	OOBJECT_METHOD_SHL,
	OOBJECT_METHOD_SHR,
	OOBJECT_METHOD_LT,
	OOBJECT_METHOD_LE,
	OOBJECT_METHOD_EQ,
	OOBJECT_METHOD_NE,
	OOBJECT_METHOD_GE,
	OOBJECT_METHOD_GT,
	OOBJECT_METHOD_BITWISE_NOT,
	OOBJECT_METHOD_BITWISE_AND,
	OOBJECT_METHOD_BITWISE_XOR,
	OOBJECT_METHOD_BITWISE_OR,

	OOBJECT_METHOD_ADD_ITEM, /* reserved */
	OOBJECT_METHOD_GET_ITEM,
	OOBJECT_METHOD_SET_ITEM,
	OOBJECT_METHOD_DEL_ITEM,
	OOBJECT_METHOD_HAS_ITEM,
	OOBJECT_METHOD_ALL_ITEM, /* [name, name, ...] or [value, value, ...] */
	OOBJECT_METHOD_MAP_ITEM, /* reutrn [[name, value], [name, value]] */

	OOBJECT_METHOD_ADD_ATTR, /* reserved */
	OOBJECT_METHOD_GET_ATTR,
	OOBJECT_METHOD_SET_ATTR,
	OOBJECT_METHOD_DEL_ATTR,
	OOBJECT_METHOD_HAS_ATTR,
	OOBJECT_METHOD_ALL_ATTR, /* return [name, name, name] */
	OOBJECT_METHOD_MAP_ATTR, /* reutrn {name: value, name: value} */

	OOBJECT_METHOD_ADD_SLICE, /* reserved */
	OOBJECT_METHOD_GET_SLICE,
	OOBJECT_METHOD_SET_SLICE,
	OOBJECT_METHOD_DEL_SLICE,

	OOBJECT_METHOD_ADD_GETTER, /* reserved */
	OOBJECT_METHOD_GET_GETTER,
	OOBJECT_METHOD_SET_GETTER,
	OOBJECT_METHOD_DEL_GETTER,

	OOBJECT_METHOD_ADD_SETTER, /* reserved */
	OOBJECT_METHOD_GET_SETTER,
	OOBJECT_METHOD_SET_SETTER,
	OOBJECT_METHOD_DEL_SETTER,

	/*
	 * different with '__call__' function attr:
	 * oobject_METHOD_CALL don't push new frame
	 * when '__call__' function call will push new frame
	 */
	OOBJECT_METHOD_CALL,

	/*
	 * we don't know C function is implemented or not oobject_METHOD_CALL,
	 * so need new method oobject_METHOD_CALLABLE.
	 * '__callable__' attr is not required when use '__call__' attr
	 */
	OOBJECT_METHOD_CALLABLE,

	OOBJECT_METHOD_SUPER,
	OOBJECT_METHOD_SUBCLASS,
	OOBJECT_METHOD_INSTANCE,

	OOBJECT_METHOD_LENGTH,  /* return ointeger */
	OOBJECT_METHOD_NUMBER,  /* return onumber  */
	OOBJECT_METHOD_STRING,  /* return ostring  */
	OOBJECT_METHOD_FORMAT,  /* return ostring  */
	OOBJECT_METHOD_INTEGER, /* return ointeger */
	OOBJECT_METHOD_BOOLEAN, /* return l_true or l_false */

	OOBJECT_METHOD_HASH, /* return ointeger */
	OOBJECT_METHOD_MARK, /* gc scan mark */

	OOBJECT_METHOD_DESTROY
};

/*
 * `l_method' also use for identify object's type
 * `l_collector` also encoded current object gc mark(see collector.c)
 */
struct oobject {
	oobject_method_t l_method;
	struct oobject *l_next;
};

/*
 * oobject naming rule:
 *
 *     oobject_[name]
 *
 * [name] is lowercase method name
 * return value and parameter always `struct oobject *' (except `argc')
 *
 *     oobject_is_[name]
 *
 * [name] is type or method
 * return value 1 success or 0 failure
 *
 *     oobject_error_[name]
 *
 * [name] is kind of error
 * return value is throwed oexception instance
 *
 *     l[type]_[name]
 *
 * [type] is specific type of oobject
 * [name] is lowercase method name
 * return value always `struct oobject *' (except `argc' or size)
 * parameter use oobject and C type
 *
 */

/*
 * if `type' is not NULL, `method' will be discard,
 * object will use type->method instead (for dynamic library type stability).
 */
void *
oobject_create(struct osic *osic, size_t size, oobject_method_t method);

int
oobject_destroy(struct osic *osic, struct oobject *object);

/*
 * copy object if size > sizeof(struct oobject)
 * memcpy object may cause gc dead loop
 */
void
oobject_copy(struct osic *osic,
             struct oobject *newobject,
             struct oobject *oldobject,
             size_t size);

struct oobject *
oobject_eq(struct osic *osic, struct oobject *a, struct oobject *b);

struct oobject *
oobject_unop(struct osic *osic, int method, struct oobject *a);

struct oobject *
oobject_binop(struct osic *osic,
              int method, struct oobject *a, struct oobject *b);

struct oobject *
oobject_mark(struct osic *osic, struct oobject *self);

struct oobject *
oobject_string(struct osic *osic, struct oobject *self);

struct oobject *
oobject_length(struct osic *osic, struct oobject *self);

struct oobject *
oobject_integer(struct osic *osic, struct oobject *self);

struct oobject *
oobject_boolean(struct osic *osic, struct oobject *self);

struct oobject *
oobject_call(struct osic *osic,
             struct oobject *self,
             int argc, struct oobject *argv[]);

struct oobject *
oobject_all_item(struct osic *osic, struct oobject *self);

struct oobject *
oobject_map_item(struct osic *osic, struct oobject *self);

struct oobject *
oobject_get_item(struct osic *osic,
                 struct oobject *self,
                 struct oobject *name);

struct oobject *
oobject_has_item(struct osic *osic,
                 struct oobject *self,
                 struct oobject *name);

struct oobject *
oobject_set_item(struct osic *osic,
                 struct oobject *self,
                 struct oobject *name,
                 struct oobject *value);

struct oobject *
oobject_del_item(struct osic *osic,
                 struct oobject *self,
                 struct oobject *name);

struct oobject *
oobject_add_attr(struct osic *osic,
                 struct oobject *self,
                 struct oobject *name,
                 struct oobject *value);

struct oobject *
oobject_get_attr(struct osic *osic,
                 struct oobject *self,
                 struct oobject *name);

struct oobject *
oobject_set_attr(struct osic *osic,
                 struct oobject *self,
                 struct oobject *name,
                 struct oobject *value);

struct oobject *
oobject_del_attr(struct osic *osic,
                 struct oobject *self,
                 struct oobject *name);

struct oobject *
oobject_has_attr(struct osic *osic,
                 struct oobject *self,
                 struct oobject *name);

struct oobject *
oobject_get_slice(struct osic *osic,
                  struct oobject *self,
                  struct oobject *start,
                  struct oobject *stop,
                  struct oobject *step);

struct oobject *
oobject_set_slice(struct osic *osic,
                  struct oobject *self,
                  struct oobject *start,
                  struct oobject *stop,
                  struct oobject *step,
                  struct oobject *value);

struct oobject *
oobject_del_slice(struct osic *osic,
                  struct oobject *self,
                  struct oobject *start,
                  struct oobject *stop,
                  struct oobject *step);

struct oobject *
oobject_get_getter(struct osic *osic,
                   struct oobject *self,
                   struct oobject *name);

struct oobject *
oobject_get_setter(struct osic *osic,
                   struct oobject *self,
                   struct oobject *name);

struct oobject *
oobject_call_attr(struct osic *osic,
                  struct oobject *self,
                  struct oobject *name,
                  int argc, struct oobject *argv[]);

int
oobject_is_integer(struct osic *osic, struct oobject *object);

int
oobject_is_pointer(struct osic *osic, struct oobject *object);

int
oobject_is_type(struct osic *osic, struct oobject *object);

int
oobject_is_karg(struct osic *osic, struct oobject *object);

int
oobject_is_varg(struct osic *osic, struct oobject *object);

int
oobject_is_vkarg(struct osic *osic, struct oobject *object);

int
oobject_is_class(struct osic *osic, struct oobject *object);

int
oobject_is_array(struct osic *osic, struct oobject *object);

int
oobject_is_number(struct osic *osic, struct oobject *object);

int
oobject_is_double(struct osic *osic, struct oobject *object);

int
oobject_is_string(struct osic *osic, struct oobject *object);

int
oobject_is_iterator(struct osic *osic, struct oobject *object);

int
oobject_is_instance(struct osic *osic, struct oobject *object);

int
oobject_is_function(struct osic *osic, struct oobject *object);

int
oobject_is_coroutine(struct osic *osic, struct oobject *object);

int
oobject_is_exception(struct osic *osic, struct oobject *object);

int
oobject_is_dictionary(struct osic *osic, struct oobject *object);

int
oobject_is_error(struct osic *osic, struct oobject *object);

int
oobject_is_equal(struct osic *osic, struct oobject *a, struct oobject *b);

struct oobject *
oobject_error(struct osic *osic, struct oobject *self, const char *fmt, ...);

struct oobject *
oobject_error_type(struct osic *osic, const char *fmt, ...);

struct oobject *
oobject_error_item(struct osic *osic, const char *fmt, ...);

struct oobject *
oobject_error_memory(struct osic *osic, const char *fmt, ...);

struct oobject *
oobject_error_runtime(struct osic *osic, const char *fmt, ...);

struct oobject *
oobject_error_argument(struct osic *osic, const char *fmt, ...);

struct oobject *
oobject_error_attribute(struct osic *osic, const char *fmt, ...);

struct oobject *
oobject_error_arithmetic(struct osic *osic, const char *fmt, ...);

void *
oobject_error_not_callable(struct osic *osic, struct oobject *object);

void *
oobject_error_not_iterable(struct osic *osic, struct oobject *object);

void *
oobject_error_not_implemented(struct osic *osic);

struct oobject *
oobject_throw(struct osic *osic, struct oobject *error);

void
oobject_print(struct osic *osic, ...);

struct oobject *
oobject_method_call(struct osic *osic,
                    struct oobject *self,
                    int method, int argc, struct oobject *argv[]);

struct oobject *
oobject_default(struct osic *osic,
                struct oobject *self,
                int method, int argc, struct oobject *argv[]);

/*
 * get_attr with default value
 * `oobject_string' -> `__string__()'
 * `oobject_length' -> `__length__()'
 *  ...
 */
struct oobject *
oobject_default_get_attr(struct osic *osic,
                         struct oobject *self,
                         struct oobject *name);

#endif /* OSIC_OOBJECT_H */
