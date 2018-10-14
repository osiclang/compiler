#ifndef OSIC_VM_H
#define OSIC_VM_H

#include <stdint.h>

#include "osic.h"
#include "lframe.h"

struct machine {
	int pc; /* program counter */
	int fp; /* frame pointer */
	int sp; /* stack pointer */

	int halt;
	int maxpc;

	int codelen;

	int framelen;
	int stacklen;
	int cpoollen;

	unsigned char *code;

	struct lframe *pause;

	struct lframe **frame;
	struct lobject **stack;
	struct lobject **cpool;

	struct lobject *exception;
};

struct machine *
machine_create(struct osic *osic);

void
machine_destroy(struct osic *osic, struct machine *machine);

void
machine_reset(struct osic *osic);

int
machine_add_const(struct osic *osic, struct lobject *object);

struct lobject *
machine_get_const(struct osic *osic, int pool);

int
machine_add_code1(struct osic *osic, int value);

int
machine_set_code1(struct osic *osic, int location, int value);

int
machine_add_code4(struct osic *osic, int value);

int
machine_set_code4(struct osic *osic, int location, int value);

struct lobject *
machine_pop_object(struct osic *osic);

void
machine_push_object(struct osic *osic, struct lobject *object);

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
machine_return_frame(struct osic *osic,
                     struct lobject *retval);

void
machine_push_frame(struct osic *osic,
                   struct lframe *frame);

struct lframe *
machine_peek_frame(struct osic *osic);

struct lframe *
machine_pop_frame(struct osic *osic);

void
machine_store_frame(struct osic *osic,
                    struct lframe *frame);

void
machine_restore_frame(struct osic *osic,
                      struct lframe *frame);

struct lobject *
machine_throw(struct osic *osic,
              struct lobject *object);

void
machine_disassemble(struct osic *osic);

#endif /* osic_VM_H */
