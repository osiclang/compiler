#include "osic.h"
#include "builtin.h"
#include "oArray.h"
#include "oString.h"
#include "oInteger.h"
#include "oIterator.h"

#include <stdio.h>
#include <string.h>

static struct oobject *
builtin_print_string_callback(struct osic *osic,
                              struct oframe *frame,
                              struct oobject *retval)
{
	struct oobject *strings;

	if (!oobject_is_string(osic, retval)) {
		const char *fmt;

		fmt = "'__string__()' return non-string";
		return oobject_error_type(osic, fmt);
	}
	strings = oframe_get_item(osic, frame, 0);

	return oarray_append(osic, strings, 1, &retval);
}

static struct oobject *
builtin_print_object(struct osic *osic,
                     long i,
                     struct oobject *objects,
                     struct oobject *strings);

static struct oobject *
builtin_print_callback(struct osic *osic,
                       struct oframe *frame,
                       struct oobject *retval)
{
	long i;
	struct oobject *offset;
	struct oobject *string;
	struct oobject *objects;
	struct oobject *strings;

	offset = oframe_get_item(osic, frame, 0);
	objects = oframe_get_item(osic, frame, 1);
	strings = oframe_get_item(osic, frame, 2);

	i = ointeger_to_long(osic, offset);
	if (i < oarray_length(osic, objects)) {
		return builtin_print_object(osic, i, objects, strings);
	}

	/* finish strings */
	for (i = 0; i < oarray_length(osic, strings) - 1; i++) {
		string = oarray_get_item(osic, strings, i);
		printf("%s ", ostring_to_cstr(osic, string));
	}
	string = oarray_get_item(osic, strings, i);
	printf("%s\n", ostring_to_cstr(osic, string));

	return osic->l_nil;
}

static struct oobject *
builtin_print_instance(struct osic *osic,
                       struct oobject *object,
                       struct oobject *strings)
{
	struct oframe *frame;
	struct oobject *function;

	function = oobject_default_get_attr(osic,
	                                    object,
	                                    osic->l_string_string);
	if (!function) {
		const char *fmt;

		fmt = "'%@' has no attribute '__string__()'";
		return oobject_error_type(osic, fmt, object);
	}

	if (oobject_is_error(osic, function)) {
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
	oframe_set_item(osic, frame, 0, strings);

	return oobject_call(osic, function, 0, NULL);
}

static struct oobject *
builtin_print_object(struct osic *osic,
                     long i,
                     struct oobject *objects,
                     struct oobject *strings)
{
	struct oframe *frame;
	struct oobject *offset;
	struct oobject *object;
	struct oobject *string;

	frame = osic_machine_push_new_frame(osic,
	                                     NULL,
	                                     NULL,
	                                     builtin_print_callback,
	                                     3);
	if (!frame) {
		return NULL;
	}

	offset = ointeger_create_from_long(osic, i + 1);
	oframe_set_item(osic, frame, 0, offset);
	oframe_set_item(osic, frame, 1, objects);
	oframe_set_item(osic, frame, 2, strings);

	object = oarray_get_item(osic, objects, i);

	/*
	 * instance object use `__string__' attr
	 */
	if (oobject_is_instance(osic, object)) {
		return builtin_print_instance(osic, object, strings);
	}

	/*
	 * non-instance object use `oobject_string'
	 */
	string = oobject_string(osic, object);

	return oarray_append(osic, strings, 1, &string);
}

static struct oobject *
builtin_print(struct osic *osic,
              struct oobject *self,
              int argc, struct oobject *argv[])
{
	struct oobject *objects;
	struct oobject *strings;

	objects = oarray_create(osic, argc, argv);
	if (!objects) {
		return NULL;
	}
	strings = oarray_create(osic, 0, NULL);
	if (!strings) {
		return NULL;
	}

	return builtin_print_object(osic, 0, objects, strings);
}

static struct oobject *
builtin_input(struct osic *osic,
              struct oobject *self,
              int argc, struct oobject *argv[])
{
	char *p;
	const char *fmt;
	const char *cstr;
	char buffer[4096];

	if (argc > 1) {
		fmt = "input() take 1 string argument";
		return oobject_error_argument(osic, fmt);
	}

	if (argc && !oobject_is_string(osic, argv[0])) {
		fmt = "input() take 1 string argument";
		return oobject_error_argument(osic, fmt);
	}

	if (argc) {
		cstr = ostring_to_cstr(osic, argv[0]);
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
			return ostring_create(osic, p, length);
		}
	}

	return osic->l_empty_string;
}

static struct oobject *
builtin_map_item_callback(struct osic *osic,
                          struct oframe *frame,
                          struct oobject *retval)
{
	long i;
	struct oobject *object;
	struct oobject *callable;
	struct oobject *iterable;

	if (!oarray_append(osic, frame->self, 1, &retval)) {
		return NULL;
	}

	callable = frame->callee;
	iterable = oframe_get_item(osic, frame, 0);
	i = oarray_length(osic, frame->self);
	if (i < oarray_length(osic, iterable)) {
		object = oarray_get_item(osic, iterable, i);
		frame = osic_machine_push_new_frame(osic,
		                                     frame->self,
		                                     callable,
		                                     builtin_map_item_callback,
		                                     1);
		if (!frame) {
			return NULL;
		}
		oframe_set_item(osic, frame, 0, iterable);

		return oobject_call(osic, callable, 1, &object);
	}

	return oarray_create(osic, 0, NULL);
}

static struct oobject *
builtin_map_array_callback(struct osic *osic,
                           struct oframe *frame,
                           struct oobject *retval)
{
	return frame->self;
}

static struct oobject *
builtin_map_array(struct osic *osic,
                  struct oobject *callable,
                  struct oobject *iterable)
{
	struct oframe *frame;
	struct oobject *array;
	struct oobject *object;

	array = oarray_create(osic, 0, NULL);
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
	oframe_set_item(osic, frame, 0, iterable);

	object = oarray_get_item(osic, iterable, 0);
	if (!object) {
		return NULL;
	}

	return oobject_call(osic, callable, 1, &object);
}

static struct oobject *
builtin_map_iterable_callback(struct osic *osic,
                              struct oframe *frame,
                              struct oobject *retval)
{
	if (frame->self != osic->l_nil) {
		return builtin_map_array(osic, frame->self, retval);
	}

	return retval;
}

static struct oobject *
builtin_map_iterable(struct osic *osic,
                     struct oobject *function,
                     struct oobject *iterable)
{
	struct oframe *frame;

	frame = osic_machine_push_new_frame(osic,
	                                     function,
	                                     NULL,
	                                     builtin_map_iterable_callback,
	                                     0);
	if (!frame) {
		return NULL;
	}

	return oiterator_to_array(osic, iterable, 0);
}

struct oobject *
builtin_map(struct osic *osic,
            struct oobject *self,
            int argc, struct oobject *argv[])
{
	struct oobject *callable;
	struct oobject *iterable;

	if (argc != 2) {
		return oobject_error_argument(osic,
		                              "'map()' require 2 arguments");
	}

	callable = argv[0];
	iterable = argv[1];
	if (oobject_is_array(osic, iterable)) {
		return builtin_map_array(osic, callable, iterable);
	}

	return builtin_map_iterable(osic, callable, iterable);
}

void
builtin_init(struct osic *osic)
{
	char *cstr;
	struct oobject *name;
	struct oobject *function;

	cstr = "map";
	name = ostring_create(osic, cstr, strlen(cstr));
	function = ofunction_create(osic, name, NULL, builtin_map);
	osic_add_global(osic, cstr, function);

	cstr = "print";
	name = ostring_create(osic, cstr, strlen(cstr));
	function = ofunction_create(osic, name, NULL, builtin_print);
	osic_add_global(osic, cstr, function);

	cstr = "input";
	name = ostring_create(osic, cstr, strlen(cstr));
	function = ofunction_create(osic, name, NULL, builtin_input);
	osic_add_global(osic, cstr, function);
}
