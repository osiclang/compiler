#include "osic.h"
#include "operative_code.h"
#include "machine.h"
#include "allocator.h"
#include "lib/garbagecollector.h"
#include "oKarg.h"
#include "oVarg.h"
#include "oVkarg.h"
#include "oArray.h"
#include "oClass.h"
#include "oSuper.h"
#include "oModule.h"
#include "oString.h"
#include "oInteger.h"
#include "oIterator.h"
#include "oDict.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>

struct machine *
machine_create(struct osic *osic)
{
	size_t size;
	struct machine *machine;

	machine = allocator_alloc(osic, sizeof(*machine));
	if (!machine) {
		return NULL;
	}
	memset(machine, 0, sizeof(*machine));
	machine->fp = -1;
	machine->sp = -1;

	machine->codelen = 4096;
	machine->cpoollen = 256;
	machine->framelen = 256;
	machine->stacklen = 256;

	size = sizeof(unsigned char) * machine->codelen;
	machine->code = allocator_alloc(osic, size);
	if (!machine->code) {
		return NULL;
	}
	memset(machine->code, 0, size);

	size = sizeof(struct oobject *) * machine->cpoollen;
	machine->cpool = allocator_alloc(osic, size);
	if (!machine->cpool) {
		return NULL;
	}
	memset(machine->cpool, 0, size);

	size = sizeof(struct oframe *) * machine->framelen;
	machine->frame = allocator_alloc(osic, size);
	if (!machine->frame) {
		return NULL;
	}
	memset(machine->frame, 0, size);

	size = sizeof(struct oobject *) * machine->stacklen;
	machine->stack = allocator_alloc(osic, size);
	if (!machine->stack) {
		return NULL;
	}
	memset(machine->stack, 0, size);

	return machine;
}

void
machine_destroy(struct osic *osic, struct machine *machine)
{
	allocator_free(osic, machine->code);
	allocator_free(osic, machine->cpool);
	allocator_free(osic, machine->frame);
	allocator_free(osic, machine->stack);
	allocator_free(osic, machine);
}

void
machine_reset(struct osic *osic)
{
	osic_machine_set_pc(osic, 0);
}

void
osic_machine_reset(struct osic *osic)
{
	machine_reset(osic);
}

int
machine_add_code1(struct osic *osic, int value)
{
	int location;
	struct machine *machine;

	machine = osic->l_machine;
	if (machine->pc + 1 >= machine->codelen) {
		size_t size;

		size = sizeof(unsigned char) * machine->codelen * 2;
		machine->code = allocator_realloc(osic, machine->code, size);
		if (!machine->code) {
			return 0;
		}

		machine->codelen *= 2;
	}

	location = machine->pc;
	machine_set_code1(osic, location, value);
	machine->pc += 1;

	return location;
}

int
machine_set_code1(struct osic *osic, int location, int operative_code)
{
	struct machine *machine;

	machine = osic->l_machine;
	machine->code[location] = (unsigned char)operative_code;

	return location;
}

int
machine_add_code4(struct osic *osic, int value)
{
	int location;
	struct machine *machine;

	machine = osic->l_machine;
	if (machine->pc + 4 >= machine->codelen) {
		size_t size;

		size = sizeof(unsigned char) * machine->codelen * 2;
		machine->code = allocator_realloc(osic, machine->code, size);
		if (!machine->code) {
			return 0;
		}

		machine->codelen *= 2;
	}
	location = machine->pc;
	machine_set_code4(osic, location, value);
	machine->pc += 4;

	return location;
}

int
machine_set_code4(struct osic *osic, int location, int value)
{
	struct machine *machine;

	machine = osic->l_machine;
	machine->code[location + 0] = (unsigned char)((value >> 24) & 0xFF);
	machine->code[location + 1] = (unsigned char)((value >> 16) & 0xFF);
	machine->code[location + 2] = (unsigned char)((value >> 8)  & 0xFF);
	machine->code[location + 3] = (unsigned char)(value         & 0xFF);

	return location;
}

int
machine_fetch_code1(struct osic *osic)
{
	struct machine *machine;

	machine = osic->l_machine;
	if (machine->pc < machine->maxpc) {
		return machine->code[machine->pc++];
	}

	printf("fetch overflow\n");
	machine->halt = 1;

	return 0;
}

int
machine_fetch_code4(struct osic *osic)
{
	int pc;
	int value;
	struct machine *machine;

	machine = osic->l_machine;
	pc = machine->pc;
	if (pc < machine->maxpc - 3) {
		value = ((machine->code[pc] << 24)   & 0xFF000000) |
		        ((machine->code[pc+1] << 16) & 0xFF0000) |
		        ((machine->code[pc+2] << 8)  & 0xFF00) |
		         (machine->code[pc+3]        & 0xFF);
		machine->pc += 4;

		return value;
	}

	printf("fetch over size\n");
	machine->halt = 1;

	return 0;
}

int
machine_add_const(struct osic *osic, struct oobject *object)
{
	int i;
	struct machine *machine;

	machine = osic->l_machine;
	for (i = 0; i < machine->cpoollen; i++) {
		if (machine->cpool[i] == NULL) {
			machine->cpool[i] = object;

			return i;
		}

		if (object == machine->cpool[i]) {
			return i;
		}

		if (oobject_is_pointer(osic, object) &&
		    oobject_is_pointer(osic, machine->cpool[i]) &&
		    object->l_method == machine->cpool[i]->l_method &&
		    oobject_is_equal(osic, object, machine->cpool[i]))
		{
			return i;
		}
	}

	return 0;
}

struct oobject *
machine_get_const(struct osic *osic, int pool)
{
	struct machine *machine;

	machine = osic->l_machine;
	return machine->cpool[pool];
}

struct oobject *
machine_stack_underflow(struct osic *osic)
{
	struct oobject *error;

	error = oobject_error_runtime(osic, "stack underflow");
	return machine_throw(osic, error);
}

struct oobject *
machine_stack_overflow(struct osic *osic)
{
	struct oobject *error;

	error = oobject_error_runtime(osic, "stack overflow");
	return machine_throw(osic, error);
}

void
machine_frame_underflow(struct osic *osic)
{
	struct machine *machine;

	machine = osic->l_machine;
	machine->halt = 1;
	printf("machine frame underflow");
}

void
machine_frame_overflow(struct osic *osic)
{
	struct machine *machine;

	machine = osic->l_machine;
	machine->halt = 1;
	printf("machine frame overflow");
}

struct oobject *
machine_out_of_memory(struct osic *osic)
{
	struct oobject *error;

	error = osic->l_out_of_memory;
	return machine_throw(osic, error);
}

int
osic_machine_get_ra(struct osic *osic)
{
	struct machine *machine;

	machine = osic->l_machine;
	if (machine->fp >= 1) {
		return machine->frame[1]->ra;
	}

	return machine->pc;
}

void
osic_machine_set_ra(struct osic *osic, int ra)
{
	struct machine *machine;

	machine = osic->l_machine;
	if (machine->fp >= 1) {
		machine->frame[1]->ra = ra;
	}
}

int
osic_machine_get_pc(struct osic *osic)
{
	struct machine *machine;

	machine = osic->l_machine;
	return machine->pc;
}

void
osic_machine_set_pc(struct osic *osic, int pc)
{
	struct machine *machine;

	machine = osic->l_machine;
	machine->pc = pc;
}

int
osic_machine_get_fp(struct osic *osic)
{
	struct machine *machine;

	machine = osic->l_machine;
	return machine->fp;
}

void
osic_machine_set_fp(struct osic *osic, int fp)
{
	struct machine *machine;

	machine = osic->l_machine;
	machine->fp = fp;
}

int
osic_machine_get_sp(struct osic *osic)
{
	struct machine *machine;

	machine = osic->l_machine;
	return machine->sp;
}

void
osic_machine_set_sp(struct osic *osic, int sp)
{
	struct machine *machine;

	machine = osic->l_machine;
	machine->sp = sp;
}

struct oframe *
osic_machine_get_frame(struct osic *osic, int fp)
{
	struct machine *machine;

	machine = osic->l_machine;
	if (fp <= machine->fp) {
		return machine->frame[fp];
	}

	return NULL;
}

void
osic_machine_set_frame(struct osic *osic, int fp, struct oframe *frame)
{
	struct machine *machine;

	machine = osic->l_machine;
	if (fp <= machine->fp) {
		machine->frame[fp] = frame;
	}
}

struct oobject *
osic_machine_get_stack(struct osic *osic, int sp)
{
	struct machine *machine;

	machine = osic->l_machine;
	if (sp <= machine->sp) {
		return machine->stack[sp];
	}

	return NULL;
}

void
machine_push_object(struct osic *osic, struct oobject *object)
{
	struct machine *machine;
	struct oobject **stack;

	machine = osic->l_machine;
	if (machine->sp < machine->stacklen - 1) {
		machine->stack[++machine->sp] = object;
	} else {
		size_t size;

		size = sizeof(struct oobject *) * machine->stacklen * 2;
		stack = allocator_realloc(osic, machine->stack, size);
		if (stack) {
			machine->stack = stack;
			machine->stacklen *= 2;
			machine->stack[++machine->sp] = object;
		} else {
			machine_stack_overflow(osic);
		}
	}
}

void
osic_machine_push_object(struct osic *osic, struct oobject *object)
{
	machine_push_object(osic, object);
}

struct oobject *
machine_pop_object(struct osic *osic)
{
	struct machine *machine;

	machine = osic->l_machine;
	if (machine->sp >= 0) {
		return machine->stack[machine->sp--];
	}

	machine_stack_underflow(osic);

	return NULL;
}

struct oobject *
osic_machine_pop_object(struct osic *osic)
{
	return machine_pop_object(osic);
}

struct oframe *
machine_push_new_frame(struct osic *osic,
                       struct oobject *self,
                       struct oobject *callee,
                       oframe_call_t callback,
                       int nlocals)
{
	struct oframe *frame;
	struct machine *machine;

	machine = osic->l_machine;
	if (machine->fp < machine->framelen) {
		frame = oframe_create(osic, self, callee, callback, nlocals);
		if (frame) {
			machine_store_frame(osic, frame);
			machine_push_frame(osic, frame);
		}

		return frame;
	}

	machine_frame_overflow(osic);

	return NULL;
}

struct oframe *
osic_machine_push_new_frame(struct osic *osic,
                             struct oobject *self,
                             struct oobject *callee,
                             oframe_call_t callback,
                             int nlocals)
{
	return machine_push_new_frame(osic, self, callee, callback, nlocals);
}

struct oobject *
machine_return_frame(struct osic *osic, struct oobject *retval)
{
	struct machine *machine;
	struct oframe *frame;

	if (!retval) {
		return osic->l_out_of_memory;
	}
	machine = osic->l_machine;
	while (machine->fp >= 0 && machine->frame[machine->fp]->callback) {
		frame = osic_machine_pop_frame(osic);
		osic_machine_restore_frame(osic, frame);
		retval = frame->callback(osic, frame, retval);

		/* make sure function has return value */
		if (!retval) {
			return osic->l_out_of_memory;
		}

		if (oobject_is_error(osic, retval)) {
			return retval;
		}
		machine_push_object(osic, retval);
	}

	return retval;
}

struct oobject *
osic_machine_return_frame(struct osic *osic, struct oobject *retval)
{
	return machine_return_frame(osic, retval);
}

void
machine_push_frame(struct osic *osic, struct oframe *frame)
{
	struct machine *machine;

	machine = osic->l_machine;
	if (machine->fp < machine->framelen - 1) {
		machine->frame[++machine->fp] = frame;
	} else {
		printf("frame overflow\n");
		machine->halt = 1;
	}
}

void
osic_machine_push_frame(struct osic *osic, struct oframe *frame)
{
	machine_push_frame(osic, frame);
}

struct oframe *
machine_peek_frame(struct osic *osic)
{
	struct machine *machine;

	machine = osic->l_machine;
	if (machine->fp >= 0) {
		return machine->frame[machine->fp];
	}

	machine_frame_underflow(osic);

	return NULL;
}

struct oframe *
osic_machine_peek_frame(struct osic *osic)
{
	return machine_peek_frame(osic);
}

struct oframe *
machine_pop_frame(struct osic *osic)
{
	struct machine *machine;

	machine = osic->l_machine;
	if (machine->fp >= 0) {
		return machine->frame[machine->fp--];
	}

	machine_frame_underflow(osic);

	return NULL;
}

struct oframe *
osic_machine_pop_frame(struct osic *osic)
{
	return machine_pop_frame(osic);
}

void
machine_store_frame(struct osic *osic, struct oframe *frame)
{
	struct machine *machine;

	machine = osic->l_machine;
	frame->sp = machine->sp;
	frame->ra = machine->pc;
}

void
osic_machine_store_frame(struct osic *osic, struct oframe *frame)
{
	machine_store_frame(osic, frame);
}

void
machine_restore_frame(struct osic *osic, struct oframe *frame)
{
	struct machine *machine;

	machine = osic->l_machine;
	machine->sp = frame->sp;
	machine->pc = frame->ra;
}

void
osic_machine_restore_frame(struct osic *osic, struct oframe *frame)
{
	machine_restore_frame(osic, frame);
}

struct oobject *
osic_machine_parse_args(struct osic *osic,
                    struct oobject *callee,
                    struct oframe *frame,
                    int define,
                    int nvalues,
                    int nparams,
                    struct oobject *params[],
                    int argc,
                    struct oobject *argv[])
{
	int i;
	int a; /* index of argv */
	int local; /* index of frame->locals */
	const char *fmt; /* error format string */

	struct oobject *key;
	struct oobject *arg;

	long v_argc;
	struct oobject *v_arg;
	struct oobject *v_args;

	int x_argc;
	struct oobject *x_argv[256];
	struct oobject *x_args;

	int x_kargc;
	struct oobject *x_kargv[512];
	struct oobject *x_kargs;

	/* too many arguments */
	if (define == 0 && argc > nparams) {
		fmt = "%@() takes %d arguments (%d given)";
		return oobject_error_argument(osic,
		                              fmt,
		                              callee,
		                              nparams,
		                              argc);
	}

	/* remove not assignable params */
	if (define == 1) {
		nparams -= 1;
	} else if (define == 2) {
		nparams -= 1;
	} else if (define == 3) {
		nparams -= 2;
	}

	if (nparams < 0) {
		return NULL;
	}

#define machine_parse_args_set_arg(arg) do {                \
	if (local < nparams) {                              \
		oframe_set_item(osic,                      \
		                frame,                      \
		                local++,                    \
		                (arg));                     \
	} else if (define == 1 || define == 3) {            \
		if (x_argc == 256) {                        \
			return NULL;                        \
		}                                           \
		x_argv[x_argc++] = (arg);                   \
	} else {                                            \
		fmt = "%@() takes %d arguments (%d given)"; \
		return oobject_error_argument(osic,        \
		                              fmt,          \
		                              callee,       \
		                              nparams,      \
		                              argc);        \
	}                                                   \
} while (0)

#define machine_parse_args_set_karg(key, arg) do {                      \
	int j;                                                          \
	int k = -1;                                                     \
	for (j = 0; j < nparams; j++) {                                 \
		if (oobject_is_equal(osic, params[j], key)) {          \
			k = j;                                          \
			break;                                          \
		}                                                       \
	}                                                               \
	if (k < 0) {                                                    \
		if (define == 2 || define == 3) {                       \
			if (x_kargc == 512) {                           \
				return NULL;                            \
			}                                               \
			x_kargv[x_kargc++] = (key);                     \
			x_kargv[x_kargc++] = (arg);                     \
		} else {                                                \
			fmt = "'%@'() don't have parameter '%@'";       \
			return oobject_error_argument(osic,            \
			                              fmt,              \
			                              callee,           \
			                              (key));           \
		}                                                       \
	} else {                                                        \
		if (oframe_get_item(osic, frame, k)) {                 \
			fmt = "'%@' set multiple value parameter '%@'"; \
			return oobject_error_argument(osic,            \
			                              fmt,              \
			                              callee,           \
			                              (key));           \
		}                                                       \
		oframe_set_item(osic, frame, k, (arg));                \
	}                                                               \
} while (0)

	local = 0;
	x_argc = 0;
	for (a = 0;
	     a < argc &&
	     !oobject_is_karg(osic, argv[a]) &&
	     !oobject_is_varg(osic, argv[a]) &&
	     !oobject_is_vkarg(osic, argv[a]);
	     a++)
	{
		machine_parse_args_set_arg(argv[a]);
	}

	if (a < argc && oobject_is_varg(osic, argv[a])) {
		v_args = ((struct ovarg *)argv[a++])->arguments;

		if (!oobject_is_array(osic, v_args)) {
			return NULL;
		}

		v_argc = oarray_length(osic, v_args);
		for (i = 0; i < v_argc; i++) {
			arg = oarray_get_item(osic, v_args, i);

			machine_parse_args_set_arg(arg);
		}
	}

	x_kargc = 0;
	for (; a < argc && oobject_is_karg(osic, argv[a]); a++) {
		key = ((struct okarg *)argv[a])->keyword;
		arg = ((struct okarg *)argv[a])->argument;

		machine_parse_args_set_karg(key, arg);
	}

	if (a < argc && oobject_is_vkarg(osic, argv[a])) {
		v_args = oobject_map_item(osic, argv[a]);

		assert(oobject_is_array(osic, v_args));

		v_argc = oarray_length(osic, v_args);
		for (i = 0; i < v_argc; i++) {
			v_arg = oarray_get_item(osic, v_args, i);

			assert(oobject_is_array(osic, v_arg));
			assert(oarray_length(osic, v_arg) == 2);

			key = oarray_get_item(osic, v_arg, 0);
			arg = oarray_get_item(osic, v_arg, 1);

			machine_parse_args_set_karg(key, arg);
		}
	}

	if (define == 1) {
		x_args = oarray_create(osic, x_argc, x_argv);
		oframe_set_item(osic, frame, nparams, x_args);
	} else if (define == 2) {
		x_kargs = odict_create(osic, x_kargc, x_kargv);
		oframe_set_item(osic, frame, nparams, x_kargs);
	} else if (define == 3) {
		x_args = oarray_create(osic, x_argc, x_argv);
		oframe_set_item(osic, frame, nparams++, x_args);

		x_kargs = odict_create(osic, x_kargc, x_kargv);
		oframe_set_item(osic, frame, nparams, x_kargs);
	}

	/* check required args */
	for (i = 0; i < nparams - nvalues; i++) {
		if (oframe_get_item(osic, frame, i) == NULL) {
			fmt = "%@() takes %d arguments (%d given)";
			return oobject_error_argument(osic,
			                              fmt,
			                              callee,
			                              nparams,
			                              i);
		}
	}

	/* set sentinel for optional args */
	for (; i < nparams; i++) {
		if (oframe_get_item(osic, frame, i) == NULL) {
			oframe_set_item(osic, frame, i, osic->l_sentinel);
		}
	}

	/* set nil for non args */
	for (; i < frame->nlocals; i++) {
		if (oframe_get_item(osic, frame, i) == NULL) {
			oframe_set_item(osic, frame, i, osic->l_nil);
		}
	}

	return NULL;
}

struct oframe *
machine_add_pause(struct osic *osic)
{
	struct oframe *frame;
	struct oframe *oldframe;
	struct machine *machine;

	machine = osic->l_machine;
	frame = osic_machine_push_new_frame(osic,
	                                     NULL,
	                                     NULL,
	                                     oframe_default_callback,
	                                     0);
	if (!frame) {
	       machine->halt = 1;

	       return NULL;
	}

	oldframe = machine->pause;
	machine->pause = frame;

	return oldframe;
}

struct oframe *
osic_machine_add_pause(struct osic *osic)
{
	return machine_add_pause(osic);
}

struct oframe *
machine_get_pause(struct osic *osic)
{
	struct machine *machine;

	machine = osic->l_machine;
	return machine->pause;
}

struct oframe *
osic_machine_get_pause(struct osic *osic)
{
	return machine_get_pause(osic);
}

struct oframe *
machine_set_pause(struct osic *osic, struct oframe *frame)
{
	struct oframe *oldframe;
	struct machine *machine;

	machine = osic->l_machine;
	oldframe = machine->pause;
	machine->pause = frame;

	return oldframe;
}

struct oframe *
osic_machine_set_pause(struct osic *osic, struct oframe *frame)
{
	return machine_set_pause(osic, frame);
}

void
machine_del_pause(struct osic *osic, struct oframe *frame)
{
	struct machine *machine;

	machine = osic->l_machine;
	machine->pause = frame;
}

void
osic_machine_del_pause(struct osic *osic, struct oframe *frame)
{
	machine_del_pause(osic, frame);
}

struct oobject *
machine_throw(struct osic *osic, struct oobject *exception)
{
	int argc;
	struct machine *machine;

	struct oframe *frame;
	struct oobject *argv[128];

	exception = oobject_throw(osic, exception);

	argc = 0;
	machine = osic->l_machine;
	while (machine->fp >= 0 && !machine->halt) {
		int l_try;

		frame = machine_peek_frame(osic);
		l_try = frame->ea;
		if (l_try) {
			machine->pc = l_try;
			machine->sp = frame->sp;
			machine->exception = exception;
			oobject_call_attr(osic,
			                  exception,
			                  ostring_create(osic, "addtrace", 8),
			                  argc,
			                  argv);

			return machine_return_frame(osic, osic->l_nil);
		}
		if (frame == machine->pause) {
			return exception;
		}
		machine_pop_frame(osic);
		machine_restore_frame(osic, frame);
		argv[argc++] = (struct oobject *)frame;
	}

	printf("Uncaught Exception: ");
	oobject_print(osic, exception, NULL);
	oobject_call_attr(osic,
	                  exception,
	                  ostring_create(osic, "addtrace", 8),
	                  argc,
	                  argv);

	oobject_call_attr(osic,
	                  exception,
	                  ostring_create(osic, "traceback", 9),
	                  0,
	                  NULL);
	machine->halt = 1;
	return machine_return_frame(osic, osic->l_nil);
}

struct oobject *
osic_machine_throw(struct osic *osic, struct oobject *exception)
{
	return machine_throw(osic, exception);
}

void
machine_halt(struct osic *osic)
{
	struct machine *machine;

	machine = osic->l_machine;
	machine->halt = 1;
}

int
osic_machine_halted(struct osic *osic)
{
        struct machine *machine;

        machine = osic->l_machine;
        return machine->halt;
}

int
machine_extend_stack(struct osic *osic)
{
	size_t size;
	struct machine *machine;
	struct oobject **stack;

	machine = osic->l_machine;
	size = sizeof(struct oobject *) * machine->stacklen * 2;
	stack = allocator_realloc(osic, machine->stack, size);
	if (stack) {
		machine->stack = stack;
		machine->stacklen *= 2;
	} else {
		machine->stacklen = 0;
		machine_halt(osic);

		return 0;
	}

	return 1;
}

struct oobject *
machine_unpack_callback(struct osic *osic,
                   struct oframe *frame,
                   struct oobject *retval)
{
	long i;
	long n;
	long unpack;
	struct oobject *last;

	n = oarray_length(osic, retval);
	unpack = ointeger_to_long(osic, oframe_get_item(osic, frame, 0));
	if (n < unpack) {
		last = osic->l_nil;
	} else {
		n -= 1;
		last = oarray_get_item(osic, retval, n);
	}

	for (i = 0; i < n; i++) {
		machine_push_object(osic, oarray_get_item(osic, retval, i));
	}
	for (; i < unpack - 1; i++) {
		machine_push_object(osic, osic->l_nil);
	}

	/* push last */
	return last;
}

struct oobject *
machine_unpack_iterable(struct osic *osic, struct oobject *iterable, int n)
{
	struct oframe *frame;

	frame = machine_push_new_frame(osic,
	                               NULL,
	                               NULL,
	                               machine_unpack_callback,
	                               1);
	if (!frame) {
		return NULL;
	}
	oframe_set_item(osic, frame, 0, ointeger_create_from_long(osic, n));

	return oiterator_to_array(osic, iterable, n);
}

struct oobject *
machine_varg_callback(struct osic *osic,
                 struct oframe *frame,
                 struct oobject *retval)
{
	return ovarg_create(osic, retval);
}

struct oobject *
machine_varg_iterable(struct osic *osic, struct oobject *iterable)
{
	struct oframe *frame;

	frame = machine_push_new_frame(osic,
	                               NULL,
	                               NULL,
	                               machine_varg_callback,
	                               0);
	if (!frame) {
		return NULL;
	}

	return oiterator_to_array(osic, iterable, 256);
}

struct oobject *
machine_call_getter(struct osic *osic,
               struct oobject *getter,
               struct oobject *self,
               struct oobject *name,
               struct oobject *value)
{
	return oobject_call(osic, getter, 1, &value);
}

struct oobject *
machine_setattr_callback(struct osic *osic,
                    struct oframe *frame,
                    struct oobject *retval)
{
	struct oobject *name;

	name = oframe_get_item(osic, frame, 0);
	oobject_set_attr(osic, frame->self, name, retval);

	return retval;
}

struct oobject *
machine_call_setter(struct osic *osic,
               struct oobject *setter,
               struct oobject *self,
               struct oobject *name,
               struct oobject *value)
{
	struct oframe *frame;

	frame = machine_push_new_frame(osic,
	                               self,
	                               0,
	                               machine_setattr_callback,
	                               1);
	if (!frame) {
		return NULL;
	}
	oframe_set_item(osic, frame, 0, name);

	return oobject_call(osic, setter, 1, &value);
}

int
osic_machine_execute(struct osic *osic)
{
	struct machine *machine;
	struct oobject *object;

	machine = osic->l_machine;
	machine->sp = -1;
	machine->fp = -1;
	machine->halt = 0;

	osic_collector_enable(osic);
	object = osic_machine_execute_loop(osic);
	if (oobject_is_error(osic, object)) {
		printf("Uncaught Exception: ");
		oobject_print(osic, object, NULL);
		oobject_call_attr(osic,
		                  object,
		                  ostring_create(osic, "traceback", 9),
		                  0,
		                  NULL);
	}
	machine->sp = -1;
	machine->fp = -1;
	collector_full(osic);

	return 1;
}

struct oobject *
osic_machine_execute_loop(struct osic *osic)
{
	int i;
	int argc;
	int operative_code;

	struct machine *machine;
	struct oframe *frame;

	struct oobject *a; /* src value 1 or base value */
	struct oobject *b; /* src value 2               */
	struct oobject *c; /* src value 3 or dest value */
	struct oobject *d; /* src value 4               */
	struct oobject *e; /* src value 5 or exception value */

	struct oobject *argv[256]; /* base value call arguments */

#define CHECK_PAUSE(retval) do {                             \
	if (machine->fp >= 0 &&                              \
	    machine->frame[machine->fp] == machine->pause) { \
		return (retval);                             \
	}                                                    \
} while (0)


#define CHECK_NULL(p) do {                      \
	if (!(p)) {                             \
		machine_out_of_memory(osic);   \
		break;                          \
	}                                       \
} while (0)                                     \

#define CHECK_ERROR(object) do {                  \
	if (oobject_is_error(osic, (object))) {  \
		e = machine_throw(osic, object); \
		CHECK_PAUSE(e);                   \
		break;                            \
	}                                         \
} while (0)                                       \

#define UNOP(m) do {                             \
	if (machine->sp >= 0) {                  \
		a = POP_OBJECT();                \
		c = oobject_unop(osic, (m), a); \
		CHECK_NULL(c);                   \
		CHECK_ERROR(c);                  \
		PUSH_OBJECT(c);                  \
	} else {                                 \
		machine_stack_underflow(osic);  \
	}                                        \
} while(0)

#define BINOP(m) do {                                \
	if (machine->sp >= 1) {                      \
		b = POP_OBJECT();                    \
		a = POP_OBJECT();                    \
		c = oobject_binop(osic, (m), a, b); \
		CHECK_NULL(c);                       \
		CHECK_ERROR(c);                      \
		PUSH_OBJECT(c);                      \
	} else {                                     \
		e = machine_stack_underflow(osic);  \
		CHECK_PAUSE(e);                      \
	}                                            \
} while(0)

#define POP_OBJECT() machine->stack[machine->sp--]

#define PUSH_OBJECT(object) do {                                  \
	if (machine->sp < machine->stacklen - 1) {                \
		machine->stack[++machine->sp] = (object);         \
	} else {                                                  \
		if (machine_extend_stack(osic)) {                \
			machine->stack[++machine->sp] = (object); \
		} else {                                          \
			e = machine_stack_overflow(osic);        \
			CHECK_PAUSE(e);                           \
		}                                                 \
	}                                                         \
} while(0)

#define CHECK_STACK(size) do {                      \
	if (machine->sp < (size) - 1) {             \
		e = machine_stack_underflow(osic); \
		CHECK_PAUSE(e);                     \
		break;                              \
	}                                           \
} while (0)                                         \

#define FETCH_CODE1() machine->code[machine->pc++]
#define FETCH_CODE4() machine_fetch_code4(osic)

#define CHECK_FETCH(size) do {                       \
	if (machine->pc + (size) > machine->maxpc) { \
		printf("fetch underflow\n");         \
		break;                               \
	}                                            \
} while (0)                                          \

#define POP_CALLBACK_FRAME(retval) do {                                     \
	while (machine->fp >= 0 && machine->frame[machine->fp]->callback) { \
		CHECK_PAUSE((retval));                                      \
		frame = machine_pop_frame(osic);                           \
		machine_restore_frame(osic, frame);                        \
		(retval) = frame->callback(osic, frame, (retval));         \
		CHECK_NULL((retval));                                       \
		CHECK_ERROR((retval));                                      \
		PUSH_OBJECT((retval));                                      \
	}                                                                   \
} while (0)

	machine = osic->l_machine;
	if (machine->fp >= 0) {
		CHECK_PAUSE(osic->l_nil);
	}
	while (!machine->halt && machine->pc < machine->maxpc) {
		operative_code = machine->code[machine->pc++];

		switch (operative_code) {
		case OPERATIVE_CODE_HALT:
			machine->fp = -1;
			machine->sp = -1;
			machine->halt = 1;
			collector_full(osic);
			break;

		case OPERATIVE_CODE_NOP:
			break;

		case OPERATIVE_CODE_POS:
			UNOP(OOBJECT_METHOD_POS);
			break;

		case OPERATIVE_CODE_NEG:
			UNOP(OOBJECT_METHOD_NEG);
			break;

		case OPERATIVE_CODE_BNOT:
			UNOP(OOBJECT_METHOD_BITWISE_NOT);
			break;

		case OPERATIVE_CODE_ADD:
			BINOP(OOBJECT_METHOD_ADD);
			break;

		case OPERATIVE_CODE_SUB:
			BINOP(OOBJECT_METHOD_SUB);
			break;

		case OPERATIVE_CODE_MUL:
			BINOP(OOBJECT_METHOD_MUL);
			break;

		case OPERATIVE_CODE_DIV:
			BINOP(OOBJECT_METHOD_DIV);
			break;

		case OPERATIVE_CODE_MOD:
			BINOP(OOBJECT_METHOD_MOD);
			break;

		case OPERATIVE_CODE_SHL:
			BINOP(OOBJECT_METHOD_SHL);
			break;

		case OPERATIVE_CODE_SHR:
			BINOP(OOBJECT_METHOD_SHR);
			break;

		case OPERATIVE_CODE_EQ:
			BINOP(OOBJECT_METHOD_EQ);
			break;

		case OPERATIVE_CODE_NE:
			BINOP(OOBJECT_METHOD_NE);
			break;

		case OPERATIVE_CODE_IN: {
			CHECK_STACK(2);
			a = POP_OBJECT();
			b = POP_OBJECT();
			c = oobject_binop(osic,
			                  OOBJECT_METHOD_HAS_ITEM,
			                  a,
			                  b);
			CHECK_NULL(c);
			CHECK_ERROR(c);
			PUSH_OBJECT(c);
			break;
		}

		case OPERATIVE_CODE_LT:
			BINOP(OOBJECT_METHOD_LT);
			break;

		case OPERATIVE_CODE_LE:
			BINOP(OOBJECT_METHOD_LE);
			break;

		case OPERATIVE_CODE_GT:
			BINOP(OOBJECT_METHOD_GT);
			break;

		case OPERATIVE_CODE_GE:
			BINOP(OOBJECT_METHOD_GE);
			break;

		case OPERATIVE_CODE_BAND:
			BINOP(OOBJECT_METHOD_BITWISE_AND);
			break;

		case OPERATIVE_CODE_BOR:
			BINOP(OOBJECT_METHOD_BITWISE_OR);
			break;

		case OPERATIVE_CODE_BXOR:
			BINOP(OOBJECT_METHOD_BITWISE_XOR);
			break;

		case OPERATIVE_CODE_LNOT:
			CHECK_STACK(1);
			a = POP_OBJECT();
			if (oobject_boolean(osic, a) == osic->l_true) {
				PUSH_OBJECT(osic->l_false);
			} else {
				PUSH_OBJECT(osic->l_true);
			}
			break;

		case OPERATIVE_CODE_POP:
			machine->sp -= 1;
			break;

		case OPERATIVE_CODE_DUP:
			CHECK_STACK(1);
			a = machine->stack[machine->sp];
			PUSH_OBJECT(a);
			break;

		case OPERATIVE_CODE_SWAP: {
			CHECK_STACK(2);
			a = POP_OBJECT();
			b = POP_OBJECT();
			PUSH_OBJECT(a);
			PUSH_OBJECT(b);
			break;
		}

		case OPERATIVE_CODE_LOAD: {
			int level;
			int local;

			CHECK_FETCH(2);
			level = FETCH_CODE1();
			local = FETCH_CODE1();
			frame = machine_peek_frame(osic);
			while (level-- > 0) {
				frame = frame->upframe;
			}
			PUSH_OBJECT(oframe_get_item(osic, frame, local));
			break;
		}

		case OPERATIVE_CODE_STORE: {
			int level;
			int local;

			CHECK_FETCH(2);
			level = FETCH_CODE1();
			local = FETCH_CODE1();
			frame = machine_peek_frame(osic);
			while (level-- > 0) {
				frame = frame->upframe;
			}
			CHECK_STACK(1);
			a = POP_OBJECT();
			c = oframe_set_item(osic, frame, local, a);
			CHECK_NULL(c);
			CHECK_ERROR(c);
			break;
		}

		case OPERATIVE_CODE_CONST: {
			CHECK_FETCH(4);
			i = FETCH_CODE4();
			PUSH_OBJECT(machine->cpool[i]);
			break;
		}

		case OPERATIVE_CODE_UNPACK: {
			int n;
			long length;

			CHECK_FETCH(1);
			n = FETCH_CODE1();
			CHECK_STACK(1);
			a = POP_OBJECT();
			if (oobject_is_array(osic, a)) {
				length = oarray_length(osic, a);
				for (i = 0; i < n && i < length; i++) {
					c = oarray_get_item(osic, a, i);
					CHECK_NULL(c);
					CHECK_ERROR(c);
					PUSH_OBJECT(c);
				}

				for (; i < n; i++) {
					PUSH_OBJECT(osic->l_nil);
				}
			} else {
				c = machine_unpack_iterable(osic, a, n);
				CHECK_NULL(c);
				CHECK_ERROR(c);
			}
			break;
		}

		case OPERATIVE_CODE_GETITEM: {
			CHECK_STACK(2);
			b = POP_OBJECT();
			a = POP_OBJECT();
			c = oobject_get_item(osic, a, b);
			if (!c) {
				c = oobject_error_item(osic,
				                       "'%@' has no item '%@'",
				                       a,
				                       b);
			}
			CHECK_ERROR(c);
			PUSH_OBJECT(c);
			break;
		}

		case OPERATIVE_CODE_SETITEM: {
			CHECK_STACK(3);
			b = POP_OBJECT();
			a = POP_OBJECT();
			c = POP_OBJECT();
			e = oobject_set_item(osic, a, b, c);
			if (!e) {
				e = oobject_error_item(osic,
				                       "'%@' has no item '%@'",
				                       a,
				                       b);
			}
			CHECK_ERROR(e);
			break;
		}

		case OPERATIVE_CODE_DELITEM: {
			CHECK_STACK(2);
			b = POP_OBJECT();
			a = POP_OBJECT();
			e = oobject_del_item(osic, a, b);
			if (!e) {
				e = oobject_error_item(osic,
				                       "'%@' has no item '%@'",
				                       a,
				                       b);
			}
			CHECK_ERROR(e);
			break;
		}

		case OPERATIVE_CODE_GETATTR: {
			struct oobject *getter;

			CHECK_STACK(2);
			b = POP_OBJECT(); /* name */
			a = POP_OBJECT(); /* object */
			c = oobject_default_get_attr(osic, a, b);
			if (!c) {
				const char *fmt;

				fmt = "'%@' has no attribute '%@'";
				c = oobject_error_attribute(osic, fmt, a, b);
			}
			CHECK_ERROR(c);

			getter = oobject_get_getter(osic, a, b);
			if (getter) {
				c = machine_call_getter(osic, getter, a, b, c);
				CHECK_NULL(c);
				CHECK_ERROR(c);
				POP_CALLBACK_FRAME(c);
			}
			PUSH_OBJECT(c);
			break;
		}

		case OPERATIVE_CODE_SETATTR: {
			struct oobject *setter;

			CHECK_STACK(3);
			b = POP_OBJECT(); /* name */
			a = POP_OBJECT(); /* object */
			c = POP_OBJECT(); /* value */
			e = oobject_set_attr(osic, a, b, c);
			if (!e) {
				const char *fmt;

				fmt = "'%@' has no attribute '%@'";
				e = oobject_error_attribute(osic, fmt, a, b);
			}
			CHECK_ERROR(e);

			setter = oobject_get_setter(osic, a, b);
			if (setter) {
				e = machine_call_setter(osic, setter, a, b, c);
				CHECK_NULL(e);
				CHECK_ERROR(e);
				POP_CALLBACK_FRAME(e);
			}
			break;
		}

		case OPERATIVE_CODE_DELATTR: {
			CHECK_STACK(2);
			b = POP_OBJECT();
			a = POP_OBJECT();
			e = oobject_del_attr(osic, a, b);
			if (!e) {
				const char *fmt;

				fmt = "'%@' has no attribute '%@'";
				e = oobject_error_attribute(osic, fmt, a, b);
			}
			CHECK_ERROR(e);
			break;
		}

		case OPERATIVE_CODE_GETSLICE: {
			CHECK_STACK(4);
			d = POP_OBJECT();
			c = POP_OBJECT();
			b = POP_OBJECT();
			a = POP_OBJECT();
			c = oobject_get_slice(osic, a, b, c, d);
			CHECK_NULL(c);
			CHECK_ERROR(c);
			PUSH_OBJECT(c);
			break;
		}

		case OPERATIVE_CODE_SETSLICE: {
			CHECK_STACK(4);
			e = POP_OBJECT();
			d = POP_OBJECT();
			c = POP_OBJECT();
			b = POP_OBJECT();
			a = POP_OBJECT();
			e = oobject_set_slice(osic, a, b, c, d, e);
			CHECK_NULL(e);
			CHECK_ERROR(e);
			break;
		}

		case OPERATIVE_CODE_DELSLICE: {
			CHECK_STACK(4);
			d = POP_OBJECT();
			c = POP_OBJECT();
			b = POP_OBJECT();
			a = POP_OBJECT();
			e = oobject_del_slice(osic, a, b, c, d);
			CHECK_NULL(e);
			CHECK_ERROR(e);
			break;
		}

		case OPERATIVE_CODE_SETGETTER: {
			CHECK_FETCH(1);
			argc = FETCH_CODE1();

			CHECK_STACK(2);
			b = POP_OBJECT(); /* name */
			a = POP_OBJECT();

			argv[0] = b;
			CHECK_STACK(argc);
			/* start from 1 */
			argc += 1;
			for (i = 1; i < argc; i++) {
				argv[i] = POP_OBJECT();
			}

			e = oobject_method_call(osic,
			                        a,
			                        OOBJECT_METHOD_SET_GETTER,
			                        argc,
			                        argv);
			CHECK_NULL(e);
			CHECK_ERROR(e);
			break;
		}

		case OPERATIVE_CODE_SETSETTER: {
			CHECK_FETCH(1);
			argc = FETCH_CODE1();

			CHECK_STACK(2);
			b = POP_OBJECT(); /* name */
			a = POP_OBJECT();

			argv[0] = b;
			CHECK_STACK(argc);
			/* start from 1 */
			argc += 1;
			for (i = 1; i < argc; i++) {
				argv[i] = POP_OBJECT();
			}

			e = oobject_method_call(osic,
			                        a,
			                        OOBJECT_METHOD_SET_SETTER,
			                        argc,
			                        argv);
			CHECK_NULL(e);
			CHECK_ERROR(e);
			break;
		}

		case OPERATIVE_CODE_JZ: {
			int address;
			CHECK_FETCH(4);
			address = FETCH_CODE4();
			CHECK_STACK(1);
			a = POP_OBJECT();
			if (oobject_boolean(osic, a) == osic->l_false) {
				machine->pc = address;
			}
			break;
		}

		case OPERATIVE_CODE_JNZ: {
			int address;
			CHECK_FETCH(4);
			address = FETCH_CODE4();
			CHECK_STACK(1);
			a = POP_OBJECT();
			if (oobject_boolean(osic, a) == osic->l_true) {
				machine->pc = address;
			}
			break;
		}

		case OPERATIVE_CODE_JMP: {
			CHECK_FETCH(4);
			machine->pc = FETCH_CODE4();
			break;
		}

		case OPERATIVE_CODE_ARRAY: {
			size_t size;
			struct oobject **items;

			CHECK_FETCH(4);
			argc = FETCH_CODE4();
			size = sizeof(struct oobject *) * argc;
			items = allocator_alloc(osic, size);
			CHECK_NULL(items);

			CHECK_STACK(argc);
			for (i = 0; i < argc; i++) {
				items[i] = POP_OBJECT();
			}
			c = oarray_create(osic, argc, items);
			allocator_free(osic, items);
			CHECK_NULL(c);
			CHECK_ERROR(c);
			PUSH_OBJECT(c);
			break;
		}

		case OPERATIVE_CODE_DICTIONARY: {
			size_t size;
			struct oobject **items;

			CHECK_FETCH(4);
			argc = FETCH_CODE4();
			size = sizeof(struct oobject *) * argc;
			items = allocator_alloc(osic, size);
			CHECK_NULL(items);

			CHECK_STACK(argc);
			for (i = 0; i < argc; i++) {
				items[i] = POP_OBJECT();
			}
			c = odict_create(osic, argc, items);
			allocator_free(osic, items);
			CHECK_NULL(c);
			CHECK_ERROR(c);
			PUSH_OBJECT(c);
			break;
		}

		case OPERATIVE_CODE_DEFINE: {
			int define;
			int address;
			int nvalues;
			int nparams;
			int nlocals;

			struct oobject *name;
			struct oobject **params;
			struct ofunction *function;

			CHECK_FETCH(8);
			define = FETCH_CODE1();
			nvalues = FETCH_CODE1();
			nparams = FETCH_CODE1();
			nlocals = FETCH_CODE1();
			address = FETCH_CODE4();

			CHECK_STACK(1);
			name = POP_OBJECT();

			/*
			 * parameters is 0...n order list,
			 * so pop order is n..0
			 */
			CHECK_STACK(nparams);
			params = argv;
			for (i = nparams; i > 0; i--) {
				params[i - 1] = POP_OBJECT();
			}

			function = ofunction_create_with_address(osic,
			                                         name,
			                                         define,
			                                         nlocals,
			                                         nparams,
			                                         nvalues,
			                                         machine->pc,
			                                         params);
			CHECK_NULL(function);
			CHECK_ERROR((struct oobject *)function);

			function->frame = machine_peek_frame(osic);
			PUSH_OBJECT((struct oobject *)function);

			machine = osic->l_machine;
			machine->pc = address; /* jmp out of function's body */
			break;
		}

		case OPERATIVE_CODE_KARG: {
			CHECK_STACK(2);
			b = POP_OBJECT();
			a = POP_OBJECT();
			c = okarg_create(osic, a, b);
			CHECK_NULL(c);
			CHECK_ERROR(c);
			PUSH_OBJECT(c);
			break;
		}

		case OPERATIVE_CODE_VARG: {
			CHECK_STACK(1);
			a = POP_OBJECT();
			if (oobject_is_array(osic, a)) {
				c = ovarg_create(osic, a);
				CHECK_NULL(c);
				CHECK_ERROR(c);
				PUSH_OBJECT(c);
			} else {
				c = machine_varg_iterable(osic, a);
				CHECK_NULL(c);
				CHECK_ERROR(c);
			}
			break;
		}

		case OPERATIVE_CODE_VKARG: {
			CHECK_STACK(1);
			a = POP_OBJECT();
			c = ovkarg_create(osic, a);
			CHECK_NULL(c);
			CHECK_ERROR(c);
			PUSH_OBJECT(c);
			break;
		}

		case OPERATIVE_CODE_CALL: {
			CHECK_FETCH(1);
			argc = FETCH_CODE1();
			CHECK_STACK(argc);
			for (i = 0; i < argc; i++) {
				argv[i] = POP_OBJECT();
			}

			a = POP_OBJECT();
			c = oobject_call(osic, a, argc, argv);
			CHECK_NULL(c);
			CHECK_ERROR(c);
			POP_CALLBACK_FRAME(c);
			break;
		}

		case OPERATIVE_CODE_TAILCALL: {
			struct oframe *newframe;
			struct oframe *oldframe;

			CHECK_FETCH(1);
			argc = FETCH_CODE1();
			CHECK_STACK(argc);
			for (i = 0; i < argc; i++) {
				argv[i] = POP_OBJECT();
			}

			/* pop the function */
			a = POP_OBJECT();
			c = oobject_call(osic, a, argc, argv);
			/*
			 * make sure callee is a osic function
			 * otherwise tailcall is just call
			 */
			if (machine_peek_frame(osic)->callee == a) {
				newframe = machine_pop_frame(osic);
				oldframe = machine_pop_frame(osic);
				newframe->ra = oldframe->ra;
				machine_push_frame(osic, newframe);
			}

			CHECK_NULL(c);
			CHECK_ERROR(c);
			POP_CALLBACK_FRAME(c);
			break;
		}

		case OPERATIVE_CODE_RETURN: {
			CHECK_STACK(1);
			a = POP_OBJECT();
			frame = machine_pop_frame(osic);
			machine_restore_frame(osic, frame);
			PUSH_OBJECT(a);
			POP_CALLBACK_FRAME(a);
			break;
		}

		case OPERATIVE_CODE_THROW:
			CHECK_STACK(1);
			a = POP_OBJECT();
			b = machine_throw(osic, a);
			CHECK_PAUSE(b);
			break;

		case OPERATIVE_CODE_TRY: {
			int address;
			CHECK_FETCH(4);
			address = FETCH_CODE4();
			frame = machine_peek_frame(osic);
			frame->ea = address;
			break;
		}

		case OPERATIVE_CODE_UNTRY: {
			frame = machine_peek_frame(osic);
			frame->ea = 0;
			break;
		}

		case OPERATIVE_CODE_LOADEXC: {
			PUSH_OBJECT(machine->exception);
			break;
		}

		case OPERATIVE_CODE_SELF: {
			frame = machine_peek_frame(osic);
			if (frame->self) {
				PUSH_OBJECT(frame->self);
			} else {
				const char *fmt;

				fmt = "'%@' not binding to instance";
				c = oobject_error_runtime(osic,
				                          fmt,
				                          frame->callee);
				e = machine_throw(osic, c);
				CHECK_PAUSE(e);
			}
			break;
		}

		case OPERATIVE_CODE_SUPER: {
			frame = machine_peek_frame(osic);
			if (frame->self) {
				c = osuper_create(osic, frame->self);
				PUSH_OBJECT(c);
			} else {
				const char *fmt;

				fmt = "'%@' not binding to instance";
				c = oobject_error_runtime(osic,
				                          fmt,
				                          frame->callee);
				e = machine_throw(osic, c);
				CHECK_PAUSE(e);
			}
			break;
		}

		case OPERATIVE_CODE_CLASS: {
			int nattrs;
			int nsupers;
			struct oobject *name;
			struct oobject *clazz;
			struct oobject *attrs[128];
			struct oobject *supers[128];

			CHECK_FETCH(2);
			nsupers = FETCH_CODE1();
			nattrs = FETCH_CODE1();
			if (nsupers > 128) {
				printf("max super is 128\n");
			}

			CHECK_STACK(nsupers + nattrs + 1);
			name = POP_OBJECT();
			for (i = 0; i < nsupers; i++) {
				supers[i] = POP_OBJECT();
			}

			for (i = 0; i < nattrs; i += 2) {
				attrs[i] = POP_OBJECT(); /* name */
				attrs[i + 1] = POP_OBJECT(); /* value */
			}

			clazz = oclass_create(osic,
			                      name,
			                      nsupers,
			                      supers,
			                      nattrs,
			                      attrs);
			CHECK_NULL(clazz);
			CHECK_ERROR(clazz);
			PUSH_OBJECT(clazz);
			break;
		}

		case OPERATIVE_CODE_MODULE: {
			int address;
			int nlocals;
			struct oobject *callee;
			struct omodule *module;

			CHECK_FETCH(5);
			nlocals = FETCH_CODE1();
			address = FETCH_CODE4();
			module = (struct omodule *)POP_OBJECT();
			callee = (struct oobject *)module;
			frame = machine_push_new_frame(osic,
			                               NULL,
			                               callee,
			                               NULL,
			                               nlocals);
			CHECK_NULL(frame);
			frame->ra = address;
			for (i = 0; i < nlocals; i++) {
				oframe_set_item(osic, frame, i, osic->l_nil);
			}
			module->frame = frame;
			module->nlocals = nlocals;
			break;
		}

		default:
			return 0;
		}

		/*
		 * we're not run gc on allocate new object
		 * Pros:
		 *     every time run stack is stable
		 *     not require a lot of barrier thing.
		 * Cons:
		 *     if one operative_code create too many objects
		 *     will use a lot of memory.
		 * keep operative_code simple and tight
		 *
		 * NOTE: may change in future
		 */
		collector_collect(osic);
	}

	return 0;
}

void
machine_disassemble(struct osic *osic)
{
	int a;
	int b;
	int c;
	int d;
	int e;
	int operative_code;
	struct machine *machine;

	machine = osic->l_machine;
	machine->pc = 0;
	while (machine->pc < machine->maxpc) {
		operative_code = machine_fetch_code1(osic);

		printf("%d: ", machine->pc-1);

		switch (operative_code) {
		case OPERATIVE_CODE_HALT:
			printf("halt\n");
			return;

		case OPERATIVE_CODE_NOP:
			printf("nop\n");
			break;

		case OPERATIVE_CODE_ADD:
			printf("add\n");
			break;

		case OPERATIVE_CODE_SUB:
			printf("sub\n");
			break;

		case OPERATIVE_CODE_MUL:
			printf("mul\n");
			break;

		case OPERATIVE_CODE_DIV:
			printf("div\n");
			break;

		case OPERATIVE_CODE_MOD:
			printf("mod\n");
			break;

		case OPERATIVE_CODE_POS:
			printf("pos\n");
			break;

		case OPERATIVE_CODE_NEG:
			printf("neg\n");
			break;

		case OPERATIVE_CODE_SHL:
			printf("shl\n");
			break;

		case OPERATIVE_CODE_SHR:
			printf("shr\n");
			break;

		case OPERATIVE_CODE_GT:
			printf("gt\n");
			break;

		case OPERATIVE_CODE_GE:
			printf("ge\n");
			break;

		case OPERATIVE_CODE_LT:
			printf("lt\n");
			break;

		case OPERATIVE_CODE_LE:
			printf("le\n");
			break;

		case OPERATIVE_CODE_EQ:
			printf("eq\n");
			break;

		case OPERATIVE_CODE_NE:
			printf("ne\n");
			break;

		case OPERATIVE_CODE_IN:
			printf("in\n");
			break;

		case OPERATIVE_CODE_BNOT:
			printf("bnot\n");
			break;

		case OPERATIVE_CODE_BAND:
			printf("band\n");
			break;

		case OPERATIVE_CODE_BXOR:
			printf("bxor\n");
			break;

		case OPERATIVE_CODE_BOR:
			printf("bor\n");
			break;

		case OPERATIVE_CODE_LNOT:
			printf("lnot\n");
			break;

		case OPERATIVE_CODE_POP:
			printf("pop\n");
			break;

		case OPERATIVE_CODE_DUP:
			printf("dup\n");
			break;

		case OPERATIVE_CODE_SWAP:
			printf("swap\n");
			break;

		case OPERATIVE_CODE_LOAD:
			a = machine_fetch_code1(osic);
			b = machine_fetch_code1(osic);
			printf("load %d %d\n", a, b);
			break;

		case OPERATIVE_CODE_STORE:
			a = machine_fetch_code1(osic);
			b = machine_fetch_code1(osic);
			printf("store %d %d\n", a, b);
			break;

		case OPERATIVE_CODE_CONST:
			a = machine_fetch_code4(osic);
			printf("const %d ; ", a);
			oobject_print(osic, machine->cpool[a], NULL);
			break;

		case OPERATIVE_CODE_UNPACK:
			a = machine_fetch_code1(osic);
			printf("unpack %d\n", a);
			break;

		case OPERATIVE_CODE_ADDITEM:
			printf("additem\n");
			break;

		case OPERATIVE_CODE_GETITEM:
			printf("getitem\n");
			break;

		case OPERATIVE_CODE_SETITEM:
			printf("setitem\n");
			break;

		case OPERATIVE_CODE_DELITEM:
			printf("delitem\n");
			break;

		case OPERATIVE_CODE_GETATTR:
			printf("getattr\n");
			break;

		case OPERATIVE_CODE_SETATTR:
			printf("setattr\n");
			break;

		case OPERATIVE_CODE_DELATTR:
			printf("delattr\n");
			break;

		case OPERATIVE_CODE_GETSLICE:
			printf("getslice\n");
			break;

		case OPERATIVE_CODE_SETSLICE:
			printf("setslice\n");
			break;

		case OPERATIVE_CODE_DELSLICE:
			printf("delslice\n");
			break;

		case OPERATIVE_CODE_JZ:
			a = machine_fetch_code4(osic);
			printf("jz %d\n", a);
			break;

		case OPERATIVE_CODE_JNZ:
			a = machine_fetch_code4(osic);
			printf("jnz %d\n", a);
			break;

		case OPERATIVE_CODE_JMP:
			a = machine_fetch_code4(osic);
			printf("jmp %d\n", a);
			break;

		case OPERATIVE_CODE_ARRAY:
			printf("array %d\n", machine_fetch_code4(osic));
			break;

		case OPERATIVE_CODE_DICTIONARY:
			printf("dictionary %d\n", machine_fetch_code4(osic));
			break;

		case OPERATIVE_CODE_DEFINE:
			a = machine_fetch_code1(osic);
			b = machine_fetch_code1(osic);
			c = machine_fetch_code1(osic);
			d = machine_fetch_code1(osic);
			e = machine_fetch_code4(osic);
			printf("define %d %d %d %d %d\n", a, b, c, d, e);
			break;

		case OPERATIVE_CODE_KARG:
			printf("karg\n");
			break;

		case OPERATIVE_CODE_VARG:
			printf("varg\n");
			break;

		case OPERATIVE_CODE_VKARG:
			printf("vkarg\n");
			break;

		case OPERATIVE_CODE_CALL:
			a = machine_fetch_code1(osic);
			printf("call %d\n", a);
			break;

		case OPERATIVE_CODE_TAILCALL:
			a = machine_fetch_code1(osic);
			printf("tailcall %d\n", a);
			break;

		case OPERATIVE_CODE_RETURN:
			printf("return\n");
			break;

		case OPERATIVE_CODE_SELF:
			printf("self\n");
			break;

		case OPERATIVE_CODE_SUPER:
			printf("super\n");
			break;

		case OPERATIVE_CODE_CLASS:
			printf("class %d %d\n",
			       machine_fetch_code1(osic),
			       machine_fetch_code1(osic));
			break;

		case OPERATIVE_CODE_MODULE:
			a = machine_fetch_code1(osic);
			b = machine_fetch_code4(osic);
			printf("module %d %d\n", a, b);
			break;

		case OPERATIVE_CODE_SETGETTER:
			printf("setgetter %d\n", machine_fetch_code1(osic));
			break;

		case OPERATIVE_CODE_SETSETTER:
			printf("setsetter\n");
			break;

		case OPERATIVE_CODE_TRY:
			a = machine_fetch_code4(osic);
			printf("try %d\n", a);
			break;

		case OPERATIVE_CODE_UNTRY:
			printf("untry\n");
			break;

		case OPERATIVE_CODE_THROW:
			printf("throw\n");
			break;

		case OPERATIVE_CODE_LOADEXC:
			printf("loadexc\n");
			break;

		default:
			printf("error\n");
			return;
		}
	}
}
