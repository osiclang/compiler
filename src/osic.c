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
#include "lib/garbagecollector.h"
#include "oNil.h"
#include "oKarg.h"
#include "oVarg.h"
#include "oVkarg.h"
#include "oTable.h"
#include "oArray.h"
#include "oSuper.h"
#include "oClass.h"
#include "oNumber.h"
#include "oString.h"
#include "oInteger.h"
#include "oModule.h"
#include "oBoolean.h"
#include "oInstance.h"
#include "oIterator.h"
#include "oSentinel.h"
#include "oCoroutine.h"
#include "oContinuation.h"
#include "oException.h"
#include "oDict.h"

#include <stdlib.h>
#include <string.h>

#define CHECK_NULL(p) do {   \
	if (!p) {            \
		return NULL; \
	}                    \
} while(0)                   \

struct oobject *
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
	osic->l_type_type = otype_type_create(osic);
	CHECK_NULL(osic->l_type_type);
	osic->l_boolean_type = oboolean_type_create(osic);
	CHECK_NULL(osic->l_boolean_type);

	/* init essential values */
	osic->l_nil = onil_create(osic);
	CHECK_NULL(osic->l_nil);
	osic->l_true = oboolean_create(osic, 1);
	CHECK_NULL(osic->l_true);
	osic->l_false = oboolean_create(osic, 0);
	CHECK_NULL(osic->l_false);
	osic->l_sentinel = osentinel_create(osic);
	CHECK_NULL(osic->l_sentinel);

	/* init rest types */
	osic->l_karg_type = okarg_type_create(osic);
	CHECK_NULL(osic->l_sentinel);
	osic->l_varg_type = ovarg_type_create(osic);
	CHECK_NULL(osic->l_sentinel);
	osic->l_vkarg_type = ovkarg_type_create(osic);
	CHECK_NULL(osic->l_sentinel);
	osic->l_table_type = otable_type_create(osic);
	CHECK_NULL(osic->l_sentinel);
	osic->l_super_type = osuper_type_create(osic);
	CHECK_NULL(osic->l_sentinel);
	osic->l_class_type = oclass_type_create(osic);
	CHECK_NULL(osic->l_sentinel);
	osic->l_frame_type = oframe_type_create(osic);
	CHECK_NULL(osic->l_sentinel);
	osic->l_array_type = oarray_type_create(osic);
	CHECK_NULL(osic->l_sentinel);
	osic->l_number_type = onumber_type_create(osic);
	CHECK_NULL(osic->l_sentinel);
	osic->l_string_type = ostring_type_create(osic);
	CHECK_NULL(osic->l_sentinel);
	osic->l_module_type = omodule_type_create(osic);
	CHECK_NULL(osic->l_sentinel);
	osic->l_integer_type = ointeger_type_create(osic);
	CHECK_NULL(osic->l_sentinel);
	osic->l_function_type = ofunction_type_create(osic);
	CHECK_NULL(osic->l_sentinel);
	osic->l_instance_type = oinstance_type_create(osic);
	CHECK_NULL(osic->l_sentinel);
	osic->l_iterator_type = oiterator_type_create(osic);
	CHECK_NULL(osic->l_sentinel);
	osic->l_coroutine_type = ocoroutine_type_create(osic);
	CHECK_NULL(osic->l_sentinel);
	osic->l_exception_type = oexception_type_create(osic);
	CHECK_NULL(osic->l_sentinel);
	osic->l_dictionary_type = odict_type_create(osic);
	CHECK_NULL(osic->l_sentinel);
	osic->l_continuation_type = ocontinuation_type_create(osic);
	CHECK_NULL(osic->l_sentinel);

	return osic->l_nil;
}

struct oobject *
osic_init_errors(struct osic *osic)
{
	char *cstr;
	struct oobject *base;
	struct oobject *name;
	struct oobject *error;

	base = (struct oobject *)osic->l_exception_type;
	osic->l_base_error = base;

	cstr = "TypeError";
	name = ostring_create(osic, cstr, strlen(cstr));
	CHECK_NULL(name);
	error = oclass_create(osic, name, 1, &base, 0, NULL);
	CHECK_NULL(error);
	osic_add_global(osic, cstr, error);
	osic->l_type_error = error;

	cstr = "ItemError";
	name = ostring_create(osic, cstr, strlen(cstr));
	CHECK_NULL(name);
	error = oclass_create(osic, name, 1, &base, 0, NULL);
	CHECK_NULL(error);
	osic_add_global(osic, cstr, error);
	osic->l_item_error = error;

	cstr = "MemoryError";
	name = ostring_create(osic, cstr, strlen(cstr));
	CHECK_NULL(name);
	error = oclass_create(osic, name, 1, &base, 0, NULL);
	CHECK_NULL(error);
	osic_add_global(osic, cstr, error);
	osic->l_memory_error = error;

	cstr = "RuntimeError";
	name = ostring_create(osic, cstr, strlen(cstr));
	CHECK_NULL(name);
	error = oclass_create(osic, name, 1, &base, 0, NULL);
	CHECK_NULL(error);
	osic_add_global(osic, cstr, error);
	osic->l_runtime_error = error;

	cstr = "ArgumentError";
	name = ostring_create(osic, cstr, strlen(cstr));
	CHECK_NULL(name);
	error = oclass_create(osic, name, 1, &base, 0, NULL);
	CHECK_NULL(error);
	osic_add_global(osic, cstr, error);
	osic->l_argument_error = error;

	cstr = "AttributeError";
	name = ostring_create(osic, cstr, strlen(cstr));
	CHECK_NULL(name);
	error = oclass_create(osic, name, 1, &base, 0, NULL);
	CHECK_NULL(error);
	osic_add_global(osic, cstr, error);
	osic->l_attribute_error = error;

	cstr = "ArithmeticError";
	name = ostring_create(osic, cstr, strlen(cstr));
	CHECK_NULL(name);
	error = oclass_create(osic, name, 1, &base, 0, NULL);
	CHECK_NULL(error);
	osic_add_global(osic, cstr, error);
	osic->l_arithmetic_error = error;

	cstr = "NotCallableError";
	name = ostring_create(osic, cstr, strlen(cstr));
	CHECK_NULL(name);
	error = oclass_create(osic, name, 1, &base, 0, NULL);
	CHECK_NULL(error);
	osic_add_global(osic, cstr, error);
	osic->l_not_callable_error = error;

	cstr = "NotIterableError";
	name = ostring_create(osic, cstr, strlen(cstr));
	CHECK_NULL(name);
	error = oclass_create(osic, name, 1, &base, 0, NULL);
	CHECK_NULL(error);
	osic_add_global(osic, cstr, error);
	osic->l_not_iterable_error = error;

	cstr = "NotImplementedError";
	name = ostring_create(osic, cstr, strlen(cstr));
	CHECK_NULL(name);
	error = oclass_create(osic, name, 1, &base, 0, NULL);
	CHECK_NULL(error);
	osic_add_global(osic, cstr, error);
	osic->l_not_implemented_error = error;

	return osic->l_nil;
}

struct oobject *
osic_init_strings(struct osic *osic)
{
	osic->l_empty_string = ostring_create(osic, NULL, 0);
	CHECK_NULL(osic->l_empty_string);
	osic->l_space_string = ostring_create(osic, " ", 1);
	CHECK_NULL(osic->l_space_string);
	osic->l_add_string = ostring_create(osic, "__add__", 7);
	CHECK_NULL(osic->l_add_string);
	osic->l_sub_string = ostring_create(osic, "__sub__", 7);
	CHECK_NULL(osic->l_sub_string);
	osic->l_mul_string = ostring_create(osic, "__mul__", 7);
	CHECK_NULL(osic->l_mul_string);
	osic->l_div_string = ostring_create(osic, "__div__", 7);
	CHECK_NULL(osic->l_div_string);
	osic->l_mod_string = ostring_create(osic, "__mod__", 7);
	CHECK_NULL(osic->l_mod_string);
	osic->l_call_string = ostring_create(osic, "__call__", 8);
	CHECK_NULL(osic->l_call_string);
	osic->l_get_item_string = ostring_create(osic, "__get_item__", 12);
	CHECK_NULL(osic->l_get_item_string);
	osic->l_set_item_string = ostring_create(osic, "__set_item__", 12);
	CHECK_NULL(osic->l_set_item_string);
	osic->l_get_attr_string = ostring_create(osic, "__get_attr__", 12);
	CHECK_NULL(osic->l_get_attr_string);
	osic->l_set_attr_string = ostring_create(osic, "__set_attr__", 12);
	CHECK_NULL(osic->l_set_attr_string);
	osic->l_del_attr_string = ostring_create(osic, "__del_attr__", 12);
	CHECK_NULL(osic->l_del_attr_string);
	osic->l_init_string = ostring_create(osic, "__init__", 8);
	CHECK_NULL(osic->l_init_string);
	osic->l_next_string = ostring_create(osic, "__next__", 8);
	CHECK_NULL(osic->l_next_string);
	osic->l_array_string = ostring_create(osic, "__array__", 9);
	CHECK_NULL(osic->l_array_string);
	osic->l_string_string = ostring_create(osic, "__string__", 10);
	CHECK_NULL(osic->l_string_string);
	osic->l_iterator_string = ostring_create(osic, "__iterator__", 12);
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

	osic->l_out_of_memory = oobject_error_memory(osic, "Out of Memory");
	CHECK_NULL(osic->l_out_of_memory);
	osic->l_modules = otable_create(osic);
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

	oobject_mark(osic, osic->l_nil);
	oobject_mark(osic, osic->l_true);
	oobject_mark(osic, osic->l_false);
	oobject_mark(osic, osic->l_sentinel);

	slots = osic->l_types_slots;
	for (i = 0; i < osic->l_types_length; i++) {
		if (slots[i].key && slots[i].key != osic->l_sentinel) {
			oobject_mark(osic, slots[i].value);
		}
	}
}

void
osic_mark_errors(struct osic *osic)
{
	oobject_mark(osic, osic->l_base_error);
	oobject_mark(osic, osic->l_type_error);
	oobject_mark(osic, osic->l_item_error);
	oobject_mark(osic, osic->l_memory_error);
	oobject_mark(osic, osic->l_runtime_error);
	oobject_mark(osic, osic->l_argument_error);
	oobject_mark(osic, osic->l_attribute_error);
	oobject_mark(osic, osic->l_arithmetic_error);
	oobject_mark(osic, osic->l_not_callable_error);
	oobject_mark(osic, osic->l_not_iterable_error);
	oobject_mark(osic, osic->l_not_implemented_error);
}

void
osic_mark_strings(struct osic *osic)
{
	oobject_mark(osic, osic->l_empty_string);
	oobject_mark(osic, osic->l_space_string);
	oobject_mark(osic, osic->l_add_string);
	oobject_mark(osic, osic->l_sub_string);
	oobject_mark(osic, osic->l_mul_string);
	oobject_mark(osic, osic->l_div_string);
	oobject_mark(osic, osic->l_mod_string);
	oobject_mark(osic, osic->l_call_string);
	oobject_mark(osic, osic->l_get_item_string);
	oobject_mark(osic, osic->l_set_item_string);
	oobject_mark(osic, osic->l_get_attr_string);
	oobject_mark(osic, osic->l_set_attr_string);
	oobject_mark(osic, osic->l_del_attr_string);
	oobject_mark(osic, osic->l_init_string);
	oobject_mark(osic, osic->l_next_string);
	oobject_mark(osic, osic->l_array_string);
	oobject_mark(osic, osic->l_string_string);
	oobject_mark(osic, osic->l_iterator_string);
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

struct oobject *
osic_get_type(struct osic *osic, oobject_method_t method)
{
	struct oobject *type;

	type = table_search(osic,
	                    (void *)(uintptr_t)method,
	                    osic->l_types_slots,
	                    osic->l_types_length,
	                    osic_type_cmp,
	                    osic_type_hash);
	if (!type && method) {
		return otype_create(osic, NULL, method, NULL);
	}

	return type;
}

int
osic_add_type(struct osic *osic, struct otype *type)
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
osic_del_type(struct osic *osic, struct otype *type)
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

struct oobject *
osic_add_global(struct osic *osic, const char *name, void *object)
{
	struct symbol *symbol;

	symbol = scope_add_symbol(osic, osic->l_global, name, SYMBOL_GLOBAL);
	symbol->cpool = machine_add_const(osic, object);

	return object;
}
