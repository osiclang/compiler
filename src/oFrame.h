#ifndef OSIC_OFRAME_H
#define OSIC_OFRAME_H

#include "oObject.h"

struct oframe;

typedef struct oobject *(*oframe_call_t)(struct osic *,
                                         struct oframe *,
                                         struct oobject *); /* retval */

struct oobject *
oframe_default_callback(struct osic *osic,
                        struct oframe *frame,
                        struct oobject *retval);

struct oframe {
	struct oobject object;

	int ra; /* return address            */
	int sp; /* previous operand sp       */
	int ea; /* exception handler address */
	int nlocals;

	struct oobject *self;
	struct oobject *callee;
	oframe_call_t callback; /* call this function after frame is poped */
	                        /* also auto return if callback is not NULL */

	struct oframe *upframe; /* up level frame for closure function */

	/* oframe is dynamic size */
	struct oobject *locals[1];
};

struct oobject *
oframe_get_item(struct osic *osic,
                struct oframe *frame,
                int local);

struct oobject *
oframe_set_item(struct osic *osic,
                struct oframe *frame,
                int local,
                struct oobject *value);

void *
oframe_create(struct osic *osic,
              struct oobject *self,
              struct oobject *callee,
              oframe_call_t callback,
              int nlocals);

struct otype *
oframe_type_create(struct osic *osic);

#endif /* OSIC_OFRAME_H */
