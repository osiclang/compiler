#ifndef OSIC_osic_H
#define OSIC_osic_H

#include <stddef.h>
#include <stdint.h>

#include "ltype.h"
#include "lframe.h"
#include "lfunction.h"

#define OSIC_NAME_MAX 256

struct lobject;

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
	 * lobject->l_method -> ltype hashtable
	 */
	void *l_types_slots;
	unsigned long l_types_count;
	unsigned long l_types_length;

	/*
	 * use lobject->method and ltype->method to identify lobject's type
	 */
	struct ltype *l_type_type;
	struct ltype *l_karg_type;
	struct ltype *l_varg_type;
	struct ltype *l_vkarg_type;
	struct ltype *l_table_type;
	struct ltype *l_super_type;
	struct ltype *l_class_type;
	struct ltype *l_frame_type;
	struct ltype *l_array_type;
	struct ltype *l_number_type;
	struct ltype *l_string_type;
	struct ltype *l_module_type;
	struct ltype *l_integer_type;
	struct ltype *l_boolean_type;
	struct ltype *l_function_type;
	struct ltype *l_instance_type;
	struct ltype *l_iterator_type;
	struct ltype *l_coroutine_type;
	struct ltype *l_exception_type;
	struct ltype *l_dictionary_type;
	struct ltype *l_continuation_type;

	struct lobject *l_nil;
	struct lobject *l_true;
	struct lobject *l_false;
	struct lobject *l_sentinel;

	struct lobject *l_modules;

	struct lobject *l_base_error;
	struct lobject *l_type_error;
	struct lobject *l_item_error;
	struct lobject *l_memory_error;
	struct lobject *l_runtime_error;
	struct lobject *l_argument_error;
	struct lobject *l_attribute_error;
	struct lobject *l_arithmetic_error;
	struct lobject *l_not_callable_error;
	struct lobject *l_not_iterable_error;
	struct lobject *l_not_implemented_error;

	/*
	 * this is an memory_error instance
	 */
	struct lobject *l_out_of_memory;

	struct lobject *l_empty_string;
	struct lobject *l_space_string;
	struct lobject *l_add_string;
	struct lobject *l_sub_string;
	struct lobject *l_mul_string;
	struct lobject *l_div_string;
	struct lobject *l_mod_string;
	struct lobject *l_call_string;
	struct lobject *l_get_item_string;
	struct lobject *l_set_item_string;
	struct lobject *l_get_attr_string;
	struct lobject *l_set_attr_string;
	struct lobject *l_del_attr_string;
	struct lobject *l_init_string;
	struct lobject *l_string_string;
	struct lobject *l_array_string;
	struct lobject *l_next_string;
	struct lobject *l_iterator_string;
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

struct lobject *
osic_get_type(struct osic *osic, lobject_method_t method);

int
osic_add_type(struct osic *osic, struct ltype *type);

void
osic_del_type(struct osic *osic, struct ltype *type);

struct lobject *
osic_add_global(struct osic *osic, const char *name, void *object);

void
osic_machine_reset(struct osic *osic);

int
osic_machine_halted(struct osic *osic);

struct lobject *
osic_machine_throw(struct osic *osic,
                    struct lobject *lobject);

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

struct lobject *
osic_machine_get_stack(struct osic *osic, int sp);

struct lobject *
osic_machine_pop_object(struct osic *osic);

void
osic_machine_push_object(struct osic *osic,
                          struct lobject *object);

struct lframe *
osic_machine_get_frame(struct osic *osic, int fp);

void
osic_machine_set_frame(struct osic *osic, int fp, struct lframe *frame);

/*
 * 1. create frame
 * 2. store frame
 * 3. push frame
 *
 * use `osic_machine_return_frame' when C function return
 */
struct lframe *
osic_machine_push_new_frame(struct osic *osic,
                             struct lobject *self,
                             struct lobject *callee,
                             lframe_call_t callback,
                             int nlocals);

/*
 * pop out all top frame with callback
 */
struct lobject *
osic_machine_return_frame(struct osic *osic,
                           struct lobject *retval);

void
osic_machine_push_frame(struct osic *osic,
                         struct lframe *frame);

struct lframe *
osic_machine_peek_frame(struct osic *osic);

struct lframe *
osic_machine_pop_frame(struct osic *osic);

void
osic_machine_store_frame(struct osic *osic,
                          struct lframe *frame);

void
osic_machine_restore_frame(struct osic *osic,
                            struct lframe *frame);

struct lframe *
osic_machine_add_pause(struct osic *osic);

struct lframe *
osic_machine_get_pause(struct osic *osic);

struct lframe *
osic_machine_set_pause(struct osic *osic,
                        struct lframe *frame);

void
osic_machine_del_pause(struct osic *osic,
                        struct lframe *frame);

struct lobject *
osic_machine_parse_args(struct osic *osic,
                         struct lobject *callee,
                         struct lframe *frame,
                         int define,
                         int nvalues, /* number of optional parameters */
                         int nparams, /* number of parameters */
                         struct lobject *params[], /* parameters name */
                         int argc,
                         struct lobject *argv[]);

int
osic_machine_execute(struct osic *osic);

struct lobject *
osic_machine_execute_loop(struct osic *osic);

void
osic_collector_enable(struct osic *osic);

void
osic_collector_disable(struct osic *osic);

int
osic_collector_enabled(struct osic *osic);

void
osic_collector_trace(struct osic *osic,
                      struct lobject *object);

void
osic_collector_untrace(struct osic *osic,
                        struct lobject *object);

void
osic_collector_mark(struct osic *osic,
                     struct lobject *object);

void
osic_collector_barrier(struct osic *osic,
                        struct lobject *a,
                        struct lobject *b);

void
osic_collector_barrierback(struct osic *osic,
                            struct lobject *a,
                            struct lobject *b);

#endif /* osic_osic_H */
