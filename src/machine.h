#ifndef OSIC_VM_H
#define OSIC_VM_H

#include <stdint.h>
#include <stdbool.h>

#include "osic.h"
#include "oFrame.h"

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

	struct oframe *pause;

	struct oframe **frame;
	struct oobject **stack;
	struct oobject **cpool;

	struct oobject *exception;
};

struct machine *
machine_create(struct osic *osic);

void
machine_destroy(struct osic *osic, struct machine *machine);

void
machine_reset(struct osic *osic);

int
machine_add_const(struct osic *osic, struct oobject *object);

struct oobject *
machine_get_const(struct osic *osic, int pool);

int
machine_add_code1(struct osic *osic, int value);

int
machine_set_code1(struct osic *osic, int location, int value);

int
machine_add_code4(struct osic *osic, int value);

int
machine_set_code4(struct osic *osic, int location, int value);

struct oobject *
machine_pop_object(struct osic *osic);

void
machine_push_object(struct osic *osic, struct oobject *object);

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
machine_return_frame(struct osic *osic,
                     struct oobject *retval);

void
machine_push_frame(struct osic *osic,
                   struct oframe *frame);

struct oframe *
machine_peek_frame(struct osic *osic);

struct oframe *
machine_pop_frame(struct osic *osic);

void
machine_store_frame(struct osic *osic,
                    struct oframe *frame);

void
machine_restore_frame(struct osic *osic,
                      struct oframe *frame);

struct oobject *
machine_throw(struct osic *osic,
              struct oobject *object);

void
machine_disassemble(struct osic *osic, bool line_numbers);

#endif /* osic_VM_H */
