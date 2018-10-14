#include "osic.h"
#include "builtin.h"
#include "larray.h"
#include "lstring.h"
#include "linteger.h"
#include "literator.h"

#include <stdio.h>
#include <string.h>

static struct lobject *
builtin_print_string_callback(struct osic *osic,
                              struct lframe *frame,
                              struct lobject *retval)
{
	struct lobject *strings;

	if (!lobject_is_string(osic, retval)) {
		const char *fmt;

		fmt = "'__string__()' return non-string";
		return lobject_error_type(osic, fmt);
	}
	strings = lframe_get_item(osic, frame, 0);

	return larray_append(osic, strings, 1, &retval);
}

static struct lobject *
builtin_print_object(struct osic *osic,
                     long i,
                     struct lobject *objects,
                     struct lobject *strings);

static struct lobject *
builtin_print_callback(struct osic *osic,
                       struct lframe *frame,
                       struct lobject *retval)
{
	long i;
	struct lobject *offset;
	struct lobject *string;
	struct lobject *objects;
	struct lobject *strings;

	offset = lframe_get_item(osic, frame, 0);
	objects = lframe_get_item(osic, frame, 1);
	strings = lframe_get_item(osic, frame, 2);

	i = linteger_to_long(osic, offset);
	if (i < larray_length(osic, objects)) {
		return builtin_print_object(osic, i, objects, strings);
	}

	/* finish strings */
	for (i = 0; i < larray_length(osic, strings) - 1; i++) {
		string = larray_get_item(osic, strings, i);
		printf("%s ", lstring_to_cstr(osic, string));
	}
	string = larray_get_item(osic, strings, i);
	printf("%s\n", lstring_to_cstr(osic, string));

	return osic->l_nil;
}

static struct lobject *
builtin_print_instance(struct osic *osic,
                       struct lobject *object,
                       struct lobject *strings)
{
	struct lframe *frame;
	struct lobject *function;

	function = lobject_default_get_attr(osic,
	                                    object,
	                                    osic->l_string_string);
	if (!function) {
		const char *fmt;

		fmt = "'%@' has no attribute '__string__()'";
		return lobject_error_type(osic, fmt, object);
	}

	if (lobject_is_error(osic, function)) {
		return function;
	}

	frame = osic_machine_push_new_frame(osic,
	                                     object,
	                                     NULL,
	                                     builtin_print_string_callback,
	                                     1);
	if (!frame) {
		return NULL;
	}
	lframe_set_item(osic, frame, 0, strings);

	return lobject_call(osic, function, 0, NULL);
}

static struct lobject *
builtin_print_object(struct osic *osic,
                     long i,
                     struct lobject *objects,
                     struct lobject *strings)
{
	struct lframe *frame;
	struct lobject *offset;
	struct lobject *object;
	struct lobject *string;

	frame = osic_machine_push_new_frame(osic,
	                                     NULL,
	                                     NULL,
	                                     builtin_print_callback,
	                                     3);
	if (!frame) {
		return NULL;
	}

	offset = linteger_create_from_long(osic, i + 1);
	lframe_set_item(osic, frame, 0, offset);
	lframe_set_item(osic, frame, 1, objects);
	lframe_set_item(osic, frame, 2, strings);

	object = larray_get_item(osic, objects, i);

	/*
	 * instance object use `__string__' attr
	 */
	if (lobject_is_instance(osic, object)) {
		return builtin_print_instance(osic, object, strings);
	}

	/*
	 * non-instance object use `lobject_string'
	 */
	string = lobject_string(osic, object);

	return larray_append(osic, strings, 1, &string);
}

static struct lobject *
builtin_print(struct osic *osic,
              struct lobject *self,
              int argc, struct lobject *argv[])
{
	struct lobject *objects;
	struct lobject *strings;

	objects = larray_create(osic, argc, argv);
	if (!objects) {
		return NULL;
	}
	strings = larray_create(osic, 0, NULL);
	if (!strings) {
		return NULL;
	}

	return builtin_print_object(osic, 0, objects, strings);
}

static struct lobject *
builtin_input(struct osic *osic,
              struct lobject *self,
              int argc, struct lobject *argv[])
{
	char *p;
	const char *fmt;
	const char *cstr;
	char buffer[4096];

	if (argc > 1) {
		fmt = "input() take 1 string argument";
		return lobject_error_argument(osic, fmt);
	}

	if (argc && !lobject_is_string(osic, argv[0])) {
		fmt = "input() take 1 string argument";
		return lobject_error_argument(osic, fmt);
	}

	if (argc) {
		cstr = lstring_to_cstr(osic, argv[0]);
		printf("%s", cstr);
	}

	p = fgets(buffer, sizeof(buffer), stdin);
	if (p) {
		long length;

		buffer[sizeof(buffer) - 1] = '\0';
		length = strlen(p);
		/* remove trail '\n' */
		while (p[length--] == '\n') {
		}
		if (length > 0) {
			return lstring_create(osic, p, length);
		}
	}

	return osic->l_empty_string;
}

static struct lobject *
builtin_map_item_callback(struct osic *osic,
                          struct lframe *frame,
                          struct lobject *retval)
{
	long i;
	struct lobject *object;
	struct lobject *callable;
	struct lobject *iterable;

	if (!larray_append(osic, frame->self, 1, &retval)) {
		return NULL;
	}

	callable = frame->callee;
	iterable = lframe_get_item(osic, frame, 0);
	i = larray_length(osic, frame->self);
	if (i < larray_length(osic, iterable)) {
		object = larray_get_item(osic, iterable, i);
		frame = osic_machine_push_new_frame(osic,
		                                     frame->self,
		                                     callable,
		                                     builtin_map_item_callback,
		                                     1);
		if (!frame) {
			return NULL;
		}
		lframe_set_item(osic, frame, 0, iterable);

		return lobject_call(osic, callable, 1, &object);
	}

	return larray_create(osic, 0, NULL);
}

static struct lobject *
builtin_map_array_callback(struct osic *osic,
                           struct lframe *frame,
                           struct lobject *retval)
{
	return frame->self;
}

static struct lobject *
builtin_map_array(struct osic *osic,
                  struct lobject *callable,
                  struct lobject *iterable)
{
	struct lframe *frame;
	struct lobject *array;
	struct lobject *object;

	array = larray_create(osic, 0, NULL);
	if (!array) {
		return NULL;
	}
	frame = osic_machine_push_new_frame(osic,
	                                     array,
	                                     NULL,
	                                     builtin_map_array_callback,
	                                     0);
	if (!frame) {
		return NULL;
	}

	frame = osic_machine_push_new_frame(osic,
	                                     array,
	                                     callable,
	                                     builtin_map_item_callback,
	                                     1);
	if (!frame) {
		return NULL;
	}
	lframe_set_item(osic, frame, 0, iterable);

	object = larray_get_item(osic, iterable, 0);
	if (!object) {
		return NULL;
	}

	return lobject_call(osic, callable, 1, &object);
}

static struct lobject *
builtin_map_iterable_callback(struct osic *osic,
                              struct lframe *frame,
                              struct lobject *retval)
{
	if (frame->self != osic->l_nil) {
		return builtin_map_array(osic, frame->self, retval);
	}

	return retval;
}

static struct lobject *
builtin_map_iterable(struct osic *osic,
                     struct lobject *function,
                     struct lobject *iterable)
{
	struct lframe *frame;

	frame = osic_machine_push_new_frame(osic,
	                                     function,
	                                     NULL,
	                                     builtin_map_iterable_callback,
	                                     0);
	if (!frame) {
		return NULL;
	}

	return literator_to_array(osic, iterable, 0);
}

struct lobject *
builtin_map(struct osic *osic,
            struct lobject *self,
            int argc, struct lobject *argv[])
{
	struct lobject *callable;
	struct lobject *iterable;

	if (argc != 2) {
		return lobject_error_argument(osic,
		                              "'map()' require 2 arguments");
	}

	callable = argv[0];
	iterable = argv[1];
	if (lobject_is_array(osic, iterable)) {
		return builtin_map_array(osic, callable, iterable);
	}

	return builtin_map_iterable(osic, callable, iterable);
}

void
builtin_init(struct osic *osic)
{
	char *cstr;
	struct lobject *name;
	struct lobject *function;

	cstr = "map";
	name = lstring_create(osic, cstr, strlen(cstr));
	function = lfunction_create(osic, name, NULL, builtin_map);
	osic_add_global(osic, cstr, function);

	cstr = "print";
	name = lstring_create(osic, cstr, strlen(cstr));
	function = lfunction_create(osic, name, NULL, builtin_print);
	osic_add_global(osic, cstr, function);

	cstr = "input";
	name = lstring_create(osic, cstr, strlen(cstr));
	function = lfunction_create(osic, name, NULL, builtin_input);
	osic_add_global(osic, cstr, function);
}
