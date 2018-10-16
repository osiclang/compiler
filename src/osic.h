#ifndef OSIC_osic_H
#define OSIC_osic_H

#include <stddef.h>
#include <stdint.h>

#include "oType.h"
#include "oFrame.h"
#include "oFunction.h"

#define OSIC_NAME_MAX 256

struct oobject;

struct osic {
	long l_random;

	void *l_arena;
	void *l_input;
	void *l_lexer;
	void *l_scope;
	void *l_global;
	void *l_generator;
	void *l_allocator;
	void *l_collector;
	void *l_machine;

	void *l_try_enclosing;
	void *l_loop_enclosing;
	void *l_stmt_enclosing;
	void *l_space_enclosing; /* variable space (function and module) */

	/*
	 * oobject->l_method -> otype hashtable
	 */
	void *l_types_slots;
	unsigned long l_types_count;
	unsigned long l_types_length;

	/*
	 * use oobject->method and otype->method to identify oobject's type
	 */
	struct otype *l_type_type;
	struct otype *l_karg_type;
	struct otype *l_varg_type;
	struct otype *l_vkarg_type;
	struct otype *l_table_type;
	struct otype *l_super_type;
	struct otype *l_class_type;
	struct otype *l_frame_type;
	struct otype *l_array_type;
	struct otype *l_number_type;
	struct otype *l_string_type;
	struct otype *l_module_type;
	struct otype *l_integer_type;
	struct otype *l_boolean_type;
	struct otype *l_function_type;
	struct otype *l_instance_type;
	struct otype *l_iterator_type;
	struct otype *l_coroutine_type;
	struct otype *l_exception_type;
	struct otype *l_dictionary_type;
	struct otype *l_continuation_type;

	struct oobject *l_nil;
	struct oobject *l_true;
	struct oobject *l_false;
	struct oobject *l_sentinel;

	struct oobject *l_modules;

	struct oobject *l_base_error;
	struct oobject *l_type_error;
	struct oobject *l_item_error;
	struct oobject *l_memory_error;
	struct oobject *l_runtime_error;
	struct oobject *l_argument_error;
	struct oobject *l_attribute_error;
	struct oobject *l_arithmetic_error;
	struct oobject *l_not_callable_error;
	struct oobject *l_not_iterable_error;
	struct oobject *l_not_implemented_error;

	/*
	 * this is an memory_error instance
	 */
	struct oobject *l_out_of_memory;

	struct oobject *l_empty_string;
	struct oobject *l_space_string;
	struct oobject *l_add_string;
	struct oobject *l_sub_string;
	struct oobject *l_mul_string;
	struct oobject *l_div_string;
	struct oobject *l_mod_string;
	struct oobject *l_call_string;
	struct oobject *l_get_item_string;
	struct oobject *l_set_item_string;
	struct oobject *l_get_attr_string;
	struct oobject *l_set_attr_string;
	struct oobject *l_del_attr_string;
	struct oobject *l_init_string;
	struct oobject *l_string_string;
	struct oobject *l_array_string;
	struct oobject *l_next_string;
	struct oobject *l_iterator_string;
};

struct osic *
osic_create();

void
osic_destroy(struct osic *osic);

int
osic_compile(struct osic *osic);

int
osic_input_set_file(struct osic *osic,
					const char *filename);

int
osic_input_set_buffer(struct osic *osic,
					  const char *filename,
					  char *buffer,
					  int length);

void *
osic_allocator_alloc(struct osic *osic, long size);

void
osic_allocator_free(struct osic *osic, void *ptr);

void *
osic_allocator_realloc(struct osic *osic, void *ptr, long size);

void
osic_mark_types(struct osic *osic);

void
osic_mark_errors(struct osic *osic);

void
osic_mark_strings(struct osic *osic);

struct oobject *
osic_get_type(struct osic *osic, oobject_method_t method);

int
osic_add_type(struct osic *osic, struct otype *type);

void
osic_del_type(struct osic *osic, struct otype *type);

struct oobject *
osic_add_global(struct osic *osic, const char *name, void *object);

void
osic_machine_reset(struct osic *osic);

int
osic_machine_halted(struct osic *osic);

struct oobject *
osic_machine_throw(struct osic *osic,
                    struct oobject *oobject);

int
osic_machine_get_pc(struct osic *osic);

void
osic_machine_set_pc(struct osic *osic, int pc);

int
osic_machine_get_fp(struct osic *osic);

void
osic_machine_set_fp(struct osic *osic, int fp);

int
osic_machine_get_sp(struct osic *osic);

void
osic_machine_set_sp(struct osic *osic, int sp);

/*
 * first function frame's ra (machine->frame[1]->ra)
 */
int
osic_machine_get_ra(struct osic *osic);

void
osic_machine_set_ra(struct osic *osic, int ra);

struct oobject *
osic_machine_get_stack(struct osic *osic, int sp);

struct oobject *
osic_machine_pop_object(struct osic *osic);

void
osic_machine_push_object(struct osic *osic,
                          struct oobject *object);

struct oframe *
osic_machine_get_frame(struct osic *osic, int fp);

void
osic_machine_set_frame(struct osic *osic, int fp, struct oframe *frame);

/*
 * 1. create frame
 * 2. store frame
 * 3. push frame
 *
 * use `osic_machine_return_frame' when C function return
 */
struct oframe *
osic_machine_push_new_frame(struct osic *osic,
                             struct oobject *self,
                             struct oobject *callee,
                             oframe_call_t callback,
                             int nlocals);

/*
 * pop out all top frame with callback
 */
struct oobject *
osic_machine_return_frame(struct osic *osic,
                           struct oobject *retval);

void
osic_machine_push_frame(struct osic *osic,
                         struct oframe *frame);

struct oframe *
osic_machine_peek_frame(struct osic *osic);

struct oframe *
osic_machine_pop_frame(struct osic *osic);

void
osic_machine_store_frame(struct osic *osic,
                          struct oframe *frame);

void
osic_machine_restore_frame(struct osic *osic,
                            struct oframe *frame);

struct oframe *
osic_machine_add_pause(struct osic *osic);

struct oframe *
osic_machine_get_pause(struct osic *osic);

struct oframe *
osic_machine_set_pause(struct osic *osic,
                        struct oframe *frame);

void
osic_machine_del_pause(struct osic *osic,
                        struct oframe *frame);

struct oobject *
osic_machine_parse_args(struct osic *osic,
                         struct oobject *callee,
                         struct oframe *frame,
                         int define,
                         int nvalues, /* number of optional parameters */
                         int nparams, /* number of parameters */
                         struct oobject *params[], /* parameters name */
                         int argc,
                         struct oobject *argv[]);

int
osic_machine_execute(struct osic *osic);

struct oobject *
osic_machine_execute_loop(struct osic *osic);

void
osic_collector_enable(struct osic *osic);

void
osic_collector_disable(struct osic *osic);

int
osic_collector_enabled(struct osic *osic);

void
osic_collector_trace(struct osic *osic,
                      struct oobject *object);

void
osic_collector_untrace(struct osic *osic,
                        struct oobject *object);

void
osic_collector_mark(struct osic *osic,
                     struct oobject *object);

void
osic_collector_barrier(struct osic *osic,
                        struct oobject *a,
                        struct oobject *b);

void
osic_collector_barrierback(struct osic *osic,
                            struct oobject *a,
                            struct oobject *b);

#endif /* osic_osic_H */
