#include "osic.h"
#include "arena.h"
#include "input.h"
#include "lexer.h"
#include "scope.h"
#include "table.h"
#include "symbol.h"
#include "syntax.h"
#include "parser.h"
#include "machine.h"
#include "compiler.h"
#include "peephole.h"
#include "generator.h"
#include "allocator.h"
#include "collector.h"
#include "lnil.h"
#include "lkarg.h"
#include "lvarg.h"
#include "lvkarg.h"
#include "ltable.h"
#include "larray.h"
#include "lsuper.h"
#include "lclass.h"
#include "lnumber.h"
#include "lstring.h"
#include "linteger.h"
#include "lmodule.h"
#include "lboolean.h"
#include "linstance.h"
#include "literator.h"
#include "lsentinel.h"
#include "lcoroutine.h"
#include "lcontinuation.h"
#include "lexception.h"
#include "ldictionary.h"

#include <stdlib.h>
#include <string.h>

#define CHECK_NULL(p) do {   \
	if (!p) {            \
		return NULL; \
	}                    \
} while(0)                   \

struct lobject *
osic_init_types(struct osic *osic)
{
	size_t size;

	size = sizeof(struct slot) * 64;
	osic->l_types_count = 0;
	osic->l_types_length = 64;
	osic->l_types_slots = osic_allocator_alloc(osic, size);
	if (!osic->l_types_slots) {
		return NULL;
	}
	memset(osic->l_types_slots, 0, size);

	/* init type and boolean's type first */
	osic->l_type_type = ltype_type_create(osic);
	CHECK_NULL(osic->l_type_type);
	osic->l_boolean_type = lboolean_type_create(osic);
	CHECK_NULL(osic->l_boolean_type);

	/* init essential values */
	osic->l_nil = lnil_create(osic);
	CHECK_NULL(osic->l_nil);
	osic->l_true = lboolean_create(osic, 1);
	CHECK_NULL(osic->l_true);
	osic->l_false = lboolean_create(osic, 0);
	CHECK_NULL(osic->l_false);
	osic->l_sentinel = lsentinel_create(osic);
	CHECK_NULL(osic->l_sentinel);

	/* init rest types */
	osic->l_karg_type = lkarg_type_create(osic);
	CHECK_NULL(osic->l_sentinel);
	osic->l_varg_type = lvarg_type_create(osic);
	CHECK_NULL(osic->l_sentinel);
	osic->l_vkarg_type = lvkarg_type_create(osic);
	CHECK_NULL(osic->l_sentinel);
	osic->l_table_type = ltable_type_create(osic);
	CHECK_NULL(osic->l_sentinel);
	osic->l_super_type = lsuper_type_create(osic);
	CHECK_NULL(osic->l_sentinel);
	osic->l_class_type = lclass_type_create(osic);
	CHECK_NULL(osic->l_sentinel);
	osic->l_frame_type = lframe_type_create(osic);
	CHECK_NULL(osic->l_sentinel);
	osic->l_array_type = larray_type_create(osic);
	CHECK_NULL(osic->l_sentinel);
	osic->l_number_type = lnumber_type_create(osic);
	CHECK_NULL(osic->l_sentinel);
	osic->l_string_type = lstring_type_create(osic);
	CHECK_NULL(osic->l_sentinel);
	osic->l_module_type = lmodule_type_create(osic);
	CHECK_NULL(osic->l_sentinel);
	osic->l_integer_type = linteger_type_create(osic);
	CHECK_NULL(osic->l_sentinel);
	osic->l_function_type = lfunction_type_create(osic);
	CHECK_NULL(osic->l_sentinel);
	osic->l_instance_type = linstance_type_create(osic);
	CHECK_NULL(osic->l_sentinel);
	osic->l_iterator_type = literator_type_create(osic);
	CHECK_NULL(osic->l_sentinel);
	osic->l_coroutine_type = lcoroutine_type_create(osic);
	CHECK_NULL(osic->l_sentinel);
	osic->l_exception_type = lexception_type_create(osic);
	CHECK_NULL(osic->l_sentinel);
	osic->l_dictionary_type = ldictionary_type_create(osic);
	CHECK_NULL(osic->l_sentinel);
	osic->l_continuation_type = lcontinuation_type_create(osic);
	CHECK_NULL(osic->l_sentinel);

	return osic->l_nil;
}

struct lobject *
osic_init_errors(struct osic *osic)
{
	char *cstr;
	struct lobject *base;
	struct lobject *name;
	struct lobject *error;

	base = (struct lobject *)osic->l_exception_type;
	osic->l_base_error = base;

	cstr = "TypeError";
	name = lstring_create(osic, cstr, strlen(cstr));
	CHECK_NULL(name);
	error = lclass_create(osic, name, 1, &base, 0, NULL);
	CHECK_NULL(error);
	osic_add_global(osic, cstr, error);
	osic->l_type_error = error;

	cstr = "ItemError";
	name = lstring_create(osic, cstr, strlen(cstr));
	CHECK_NULL(name);
	error = lclass_create(osic, name, 1, &base, 0, NULL);
	CHECK_NULL(error);
	osic_add_global(osic, cstr, error);
	osic->l_item_error = error;

	cstr = "MemoryError";
	name = lstring_create(osic, cstr, strlen(cstr));
	CHECK_NULL(name);
	error = lclass_create(osic, name, 1, &base, 0, NULL);
	CHECK_NULL(error);
	osic_add_global(osic, cstr, error);
	osic->l_memory_error = error;

	cstr = "RuntimeError";
	name = lstring_create(osic, cstr, strlen(cstr));
	CHECK_NULL(name);
	error = lclass_create(osic, name, 1, &base, 0, NULL);
	CHECK_NULL(error);
	osic_add_global(osic, cstr, error);
	osic->l_runtime_error = error;

	cstr = "ArgumentError";
	name = lstring_create(osic, cstr, strlen(cstr));
	CHECK_NULL(name);
	error = lclass_create(osic, name, 1, &base, 0, NULL);
	CHECK_NULL(error);
	osic_add_global(osic, cstr, error);
	osic->l_argument_error = error;

	cstr = "AttributeError";
	name = lstring_create(osic, cstr, strlen(cstr));
	CHECK_NULL(name);
	error = lclass_create(osic, name, 1, &base, 0, NULL);
	CHECK_NULL(error);
	osic_add_global(osic, cstr, error);
	osic->l_attribute_error = error;

	cstr = "ArithmeticError";
	name = lstring_create(osic, cstr, strlen(cstr));
	CHECK_NULL(name);
	error = lclass_create(osic, name, 1, &base, 0, NULL);
	CHECK_NULL(error);
	osic_add_global(osic, cstr, error);
	osic->l_arithmetic_error = error;

	cstr = "NotCallableError";
	name = lstring_create(osic, cstr, strlen(cstr));
	CHECK_NULL(name);
	error = lclass_create(osic, name, 1, &base, 0, NULL);
	CHECK_NULL(error);
	osic_add_global(osic, cstr, error);
	osic->l_not_callable_error = error;

	cstr = "NotIterableError";
	name = lstring_create(osic, cstr, strlen(cstr));
	CHECK_NULL(name);
	error = lclass_create(osic, name, 1, &base, 0, NULL);
	CHECK_NULL(error);
	osic_add_global(osic, cstr, error);
	osic->l_not_iterable_error = error;

	cstr = "NotImplementedError";
	name = lstring_create(osic, cstr, strlen(cstr));
	CHECK_NULL(name);
	error = lclass_create(osic, name, 1, &base, 0, NULL);
	CHECK_NULL(error);
	osic_add_global(osic, cstr, error);
	osic->l_not_implemented_error = error;

	return osic->l_nil;
}

struct lobject *
osic_init_strings(struct osic *osic)
{
	osic->l_empty_string = lstring_create(osic, NULL, 0);
	CHECK_NULL(osic->l_empty_string);
	osic->l_space_string = lstring_create(osic, " ", 1);
	CHECK_NULL(osic->l_space_string);
	osic->l_add_string = lstring_create(osic, "__add__", 7);
	CHECK_NULL(osic->l_add_string);
	osic->l_sub_string = lstring_create(osic, "__sub__", 7);
	CHECK_NULL(osic->l_sub_string);
	osic->l_mul_string = lstring_create(osic, "__mul__", 7);
	CHECK_NULL(osic->l_mul_string);
	osic->l_div_string = lstring_create(osic, "__div__", 7);
	CHECK_NULL(osic->l_div_string);
	osic->l_mod_string = lstring_create(osic, "__mod__", 7);
	CHECK_NULL(osic->l_mod_string);
	osic->l_call_string = lstring_create(osic, "__call__", 8);
	CHECK_NULL(osic->l_call_string);
	osic->l_get_item_string = lstring_create(osic, "__get_item__", 12);
	CHECK_NULL(osic->l_get_item_string);
	osic->l_set_item_string = lstring_create(osic, "__set_item__", 12);
	CHECK_NULL(osic->l_set_item_string);
	osic->l_get_attr_string = lstring_create(osic, "__get_attr__", 12);
	CHECK_NULL(osic->l_get_attr_string);
	osic->l_set_attr_string = lstring_create(osic, "__set_attr__", 12);
	CHECK_NULL(osic->l_set_attr_string);
	osic->l_del_attr_string = lstring_create(osic, "__del_attr__", 12);
	CHECK_NULL(osic->l_del_attr_string);
	osic->l_init_string = lstring_create(osic, "__init__", 8);
	CHECK_NULL(osic->l_init_string);
	osic->l_next_string = lstring_create(osic, "__next__", 8);
	CHECK_NULL(osic->l_next_string);
	osic->l_array_string = lstring_create(osic, "__array__", 9);
	CHECK_NULL(osic->l_array_string);
	osic->l_string_string = lstring_create(osic, "__string__", 10);
	CHECK_NULL(osic->l_string_string);
	osic->l_iterator_string = lstring_create(osic, "__iterator__", 12);
	CHECK_NULL(osic->l_iterator_string);

	return osic->l_nil;
}

#undef CHECK_NULL
#define CHECK_NULL(p) do { \
	if (!p) {          \
		goto err;  \
	}                  \
} while(0)                 \

struct osic *
osic_create()
{
	struct osic *osic;

	osic = malloc(sizeof(*osic));
	if (!osic) {
		return NULL;
	}
	memset(osic, 0, sizeof(*osic));

	/*
	 * random number for seeding other module
	 * 0x4c454d9d == ('L'<<24) + ('E'<<16) + ('M'<<8) + 'O' + 'N')
	 */
#ifdef WINDOWS
	srand(0x4c454d9d);
	osic->l_random = rand();
#else
	srandom(0x4c454d9d);
	osic->l_random = random();
#endif
	osic->l_allocator = allocator_create(osic);
	CHECK_NULL(osic->l_allocator);

	osic->l_arena = arena_create(osic);
	CHECK_NULL(osic->l_arena);
	osic->l_input = input_create(osic);
	CHECK_NULL(osic->l_input);
	osic->l_lexer = lexer_create(osic);
	CHECK_NULL(osic->l_lexer);

	osic->l_global = osic_allocator_alloc(osic, sizeof(struct scope));
	CHECK_NULL(osic->l_global);
	memset(osic->l_global, 0, sizeof(struct scope));

	osic->l_generator = generator_create(osic);
	CHECK_NULL(osic->l_generator);

	osic->l_collector = collector_create(osic);
	CHECK_NULL(osic->l_collector);
	osic->l_machine = machine_create(osic);
	CHECK_NULL(osic->l_machine);

	CHECK_NULL(osic_init_types(osic));
	CHECK_NULL(osic_init_errors(osic));
	CHECK_NULL(osic_init_strings(osic));

	osic->l_out_of_memory = lobject_error_memory(osic, "Out of Memory");
	CHECK_NULL(osic->l_out_of_memory);
	osic->l_modules = ltable_create(osic);
	CHECK_NULL(osic->l_modules);

	return osic;
err:
	osic_destroy(osic);

	return NULL;
}

void
osic_destroy(struct osic *osic)
{
	input_destroy(osic, osic->l_input);
	osic->l_input = NULL;

	lexer_destroy(osic, osic->l_lexer);
	osic->l_lexer = NULL;

	machine_destroy(osic, osic->l_machine);
	osic->l_machine = NULL;

	arena_destroy(osic, osic->l_arena);
	osic->l_arena = NULL;

	collector_destroy(osic, osic->l_collector);
	osic->l_collector = NULL;

	osic_allocator_free(osic, osic->l_types_slots);
	osic->l_types_slots = NULL;

	allocator_destroy(osic, osic->l_allocator);
	osic->l_allocator = NULL;

	free(osic);
}

#undef CHECK_NULL

int
osic_compile(struct osic *osic)
{
	struct syntax *node;

	lexer_next_token(osic);

	node = parser_parse(osic);
	if (!node) {
		fprintf(stderr, "osic: syntax error\n");

		return 0;
	}

	if (!compiler_compile(osic, node)) {
		fprintf(stderr, "osic: syntax error\n");

		return 0;
	}
	peephole_optimize(osic);

	machine_reset(osic);
	generator_emit(osic);
	collector_full(osic);

	return 1;
}

void
osic_mark_types(struct osic *osic)
{
	unsigned long i;
	struct slot *slots;

	lobject_mark(osic, osic->l_nil);
	lobject_mark(osic, osic->l_true);
	lobject_mark(osic, osic->l_false);
	lobject_mark(osic, osic->l_sentinel);

	slots = osic->l_types_slots;
	for (i = 0; i < osic->l_types_length; i++) {
		if (slots[i].key && slots[i].key != osic->l_sentinel) {
			lobject_mark(osic, slots[i].value);
		}
	}
}

void
osic_mark_errors(struct osic *osic)
{
	lobject_mark(osic, osic->l_base_error);
	lobject_mark(osic, osic->l_type_error);
	lobject_mark(osic, osic->l_item_error);
	lobject_mark(osic, osic->l_memory_error);
	lobject_mark(osic, osic->l_runtime_error);
	lobject_mark(osic, osic->l_argument_error);
	lobject_mark(osic, osic->l_attribute_error);
	lobject_mark(osic, osic->l_arithmetic_error);
	lobject_mark(osic, osic->l_not_callable_error);
	lobject_mark(osic, osic->l_not_iterable_error);
	lobject_mark(osic, osic->l_not_implemented_error);
}

void
osic_mark_strings(struct osic *osic)
{
	lobject_mark(osic, osic->l_empty_string);
	lobject_mark(osic, osic->l_space_string);
	lobject_mark(osic, osic->l_add_string);
	lobject_mark(osic, osic->l_sub_string);
	lobject_mark(osic, osic->l_mul_string);
	lobject_mark(osic, osic->l_div_string);
	lobject_mark(osic, osic->l_mod_string);
	lobject_mark(osic, osic->l_call_string);
	lobject_mark(osic, osic->l_get_item_string);
	lobject_mark(osic, osic->l_set_item_string);
	lobject_mark(osic, osic->l_get_attr_string);
	lobject_mark(osic, osic->l_set_attr_string);
	lobject_mark(osic, osic->l_del_attr_string);
	lobject_mark(osic, osic->l_init_string);
	lobject_mark(osic, osic->l_next_string);
	lobject_mark(osic, osic->l_array_string);
	lobject_mark(osic, osic->l_string_string);
	lobject_mark(osic, osic->l_iterator_string);
}

static int
osic_type_cmp(struct osic *osic, void *a, void *b)
{
	return a == b;
}

static unsigned long
osic_type_hash(struct osic *osic, void *key)
{
	return (unsigned long)key;
}

struct lobject *
osic_get_type(struct osic *osic, lobject_method_t method)
{
	struct lobject *type;

	type = table_search(osic,
	                    (void *)(uintptr_t)method,
	                    osic->l_types_slots,
	                    osic->l_types_length,
	                    osic_type_cmp,
	                    osic_type_hash);
	if (!type && method) {
		return ltype_create(osic, NULL, method, NULL);
	}

	return type;
}

int
osic_add_type(struct osic *osic, struct ltype *type)
{
	void *slots;
	void *method;
	size_t size;
	unsigned long count;
	unsigned long length;

	slots = osic->l_types_slots;
	count  = osic->l_types_count;
	length = osic->l_types_length;
	method = (void *)(uintptr_t)type->method;
	osic->l_types_count += table_insert(osic,
	                                     method,
	                                     type,
	                                     slots,
	                                     length,
	                                     osic_type_cmp,
	                                     osic_type_hash);

	if (TABLE_LOAD_FACTOR(count + 1) >= length) {
		length = TABLE_GROW_FACTOR(length);
		size = sizeof(struct slot) * length;
		slots = osic_allocator_alloc(osic, size);
		if (!slots) {
			return 0;
		}
		memset(slots, 0, size);

		table_rehash(osic,
		             osic->l_types_slots,
		             osic->l_types_length,
		             slots,
		             length,
		             osic_type_cmp,
		             osic_type_hash);
		osic_allocator_free(osic, osic->l_types_slots);
		osic->l_types_slots = slots;
		osic->l_types_length = length;
	}

	return 1;
}

void
osic_del_type(struct osic *osic, struct ltype *type)
{
	if (osic->l_types_slots) {
		table_delete(osic,
		             (void *)(uintptr_t)type->method,
		             osic->l_types_slots,
		             osic->l_types_length,
		             osic_type_cmp,
		             osic_type_hash);
	}
}

struct lobject *
osic_add_global(struct osic *osic, const char *name, void *object)
{
	struct symbol *symbol;

	symbol = scope_add_symbol(osic, osic->l_global, name, SYMBOL_GLOBAL);
	symbol->cpool = machine_add_const(osic, object);

	return object;
}
