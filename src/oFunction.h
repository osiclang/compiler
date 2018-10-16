#ifndef OSIC_OFUNCTION_H
#define OSIC_OFUNCTION_H

#include "oObject.h"

struct oframe;

typedef struct oobject *(*ofunction_call_t)(struct osic *,
                                            struct oobject *,
                                            int,
                                            struct oobject *[]);

struct ofunction {
	struct oobject object;

	/*
	 * define values:
	 *
	 * 0, fixed argument function 'define func(var a, var b, var c)'
	 *    a, b and c take exactly argument
	 *
	 * 1, variable argument function 'define func(var a, var b, var *c)'
	 *    argument after b into array c
	 *
	 * 2, variable keyword argument function
	 *    'define func(var a, var b, var **c)'
	 *    keyword argument after b into dictionary c
	 *
	 * 3, variable argument and keyword argument function
	 *    'define func(var a, var *b, var **c)'
	 *    all non-keyword argument after a into array b
	 *    all keyword argument after non-keyword into dictionary c
	 */
	unsigned char define;
	unsigned char nlocals; /* all local variable (include parameters) */
	unsigned char nparams; /* number of parameters */
	unsigned char nvalues; /* number of parameters has default values */
	                       /* always nlocals >= nparams >= nvalues */

	int address; /* bytecode entry address */

	struct oframe *frame;
	struct oobject *name;
	struct oobject *self;
	ofunction_call_t callback; /* C function pointer */

	struct oobject *params[1]; /* parameters name reverse order */
};

void *
ofunction_bind(struct osic *osic,
               struct oobject *function,
               struct oobject *self);

void *
ofunction_create(struct osic *osic,
                 struct oobject *name,
                 struct oobject *self,
                 ofunction_call_t callback);

void *
ofunction_create_with_address(struct osic *osic,
                              struct oobject *name,
                              int define,
                              int nlocals,
                              int nparams,
                              int nvalues,
                              int address,
                              struct oobject *params[]);

struct otype *
ofunction_type_create(struct osic *osic);

#endif /* OSIC_OFUNCTION_H */
