#include "osic.h"
#include "operative_code.h"
#include "machine.h"
#include "generator.h"

#define IS_JZ(a) ((a) && IS_OPCODE(a, OPERATIVE_CODE_JZ))
#define IS_JNZ(a) ((a) && IS_OPCODE(a, OPERATIVE_CODE_JNZ))
#define IS_JMP(a) ((a) && IS_OPCODE(a, OPERATIVE_CODE_JMP))
#define IS_DUP(a) ((a) && IS_OPCODE(a, OPERATIVE_CODE_DUP))
#define IS_POP(a) ((a) && IS_OPCODE(a, OPERATIVE_CODE_POP))
#define IS_NOP(a) ((a) && IS_OPCODE(a, OPERATIVE_CODE_NOP))
#define IS_LOAD(a) ((a) && IS_OPCODE(a, OPERATIVE_CODE_LOAD))
#define IS_STORE(a) ((a) && IS_OPCODE(a, OPERATIVE_CODE_STORE))
#define IS_CONST(a) ((a) && IS_OPCODE(a, OPERATIVE_CODE_CONST))
#define IS_LABEL(a) ((a) && IS_OPCODE(a, -1))
#define IS_OPCODE(a,b) ((a) && (a)->operative_code == (b))

static struct generator_code *
peephole_rewrite_unop(struct osic *osic,
                      struct generator_code *a)
{
	/*
	 *    CONST a
	 *    OP
	 * ->
	 *    CONST `OP a'
	 */

	int pool;
	struct generator_code *b;
	struct lobject *result;

#define UNOP(m) do {                                                       \
	result = lobject_unop(osic,                                       \
	                      (m),                                         \
	                      machine_get_const(osic, b->arg[0]->value)); \
	pool = machine_add_const(osic, result);                           \
	if (lobject_is_error(osic, result)) {                             \
		return NULL;                                               \
	}                                                                  \
	a->operative_code = OPERATIVE_CODE_CONST;                                          \
	a->arg[0] = generator_make_arg(osic, 0, 4, pool);                 \
	generator_delete_code(osic, b);                                   \
	return a;                                                          \
} while(0)

	if (!IS_CONST(a->prev)) {
		return NULL;
	}
	b = a->prev;

	switch (a->operative_code) {
	case OPERATIVE_CODE_POS:
		UNOP(LOBJECT_METHOD_POS);
		break;

	case OPERATIVE_CODE_NEG:
		UNOP(LOBJECT_METHOD_NEG);
		break;

	case OPERATIVE_CODE_BNOT:
		UNOP(LOBJECT_METHOD_BITWISE_NOT);
		break;

	case OPERATIVE_CODE_LNOT:
		result = machine_get_const(osic, b->arg[0]->value);
		if (lobject_boolean(osic, result) == osic->l_true) {
			pool = machine_add_const(osic, osic->l_false);
		} else {
			pool = machine_add_const(osic, osic->l_true);
		}
		a->operative_code = OPERATIVE_CODE_CONST;
		a->arg[0] = generator_make_arg(osic, 0, 4, pool);
		generator_delete_code(osic, b);

		return a;

	default:
		return NULL;
	}
}

static struct generator_code *
peephole_rewrite_binop(struct osic *osic,
                       struct generator_code *c)
{
	/*
	 *    CONST a
	 *    CONST b
	 *    OP
	 * ->
	 *    CONST `a OP b'
	 */

	int pool;
	struct lobject *result;
	struct generator_code *a;
	struct generator_code *b;

#define BINOP(m) do {                                                       \
	result = lobject_binop(osic,                                       \
	                       (m),                                         \
	                       machine_get_const(osic, b->arg[0]->value),  \
	                       machine_get_const(osic, c->arg[0]->value)); \
	pool = machine_add_const(osic, result);                            \
	if (lobject_is_error(osic, result)) {                              \
		return NULL;                                                \
	}                                                                   \
	a->operative_code = OPERATIVE_CODE_CONST;                                           \
	a->arg[0] = generator_make_arg(osic, 0, 4, pool);                  \
	generator_delete_code(osic, b);                                    \
	generator_delete_code(osic, c);                                    \
	return a;                                                           \
} while(0)

	if (!IS_CONST(c->prev)) {
		return NULL;
	}
	b = c->prev;

	if (!IS_CONST(b->prev)) {
		return NULL;
	}
	a = b->prev;

	switch (a->operative_code) {
	case OPERATIVE_CODE_ADD:
		BINOP(LOBJECT_METHOD_ADD);
		break;

	case OPERATIVE_CODE_SUB:
		BINOP(LOBJECT_METHOD_SUB);
		break;

	case OPERATIVE_CODE_MUL:
		BINOP(LOBJECT_METHOD_MUL);
		break;

	case OPERATIVE_CODE_DIV:
		BINOP(LOBJECT_METHOD_DIV);
		break;

	case OPERATIVE_CODE_MOD:
		BINOP(LOBJECT_METHOD_MOD);
		break;

	case OPERATIVE_CODE_SHL:
		BINOP(LOBJECT_METHOD_SHL);
		break;

	case OPERATIVE_CODE_SHR:
		BINOP(LOBJECT_METHOD_SHR);
		break;

	case OPERATIVE_CODE_EQ:
		BINOP(LOBJECT_METHOD_EQ);
		break;

	case OPERATIVE_CODE_NE:
		BINOP(LOBJECT_METHOD_NE);
		break;

	case OPERATIVE_CODE_LT:
		BINOP(LOBJECT_METHOD_LT);
		break;

	case OPERATIVE_CODE_LE:
		BINOP(LOBJECT_METHOD_LE);
		break;

	case OPERATIVE_CODE_GT:
		BINOP(LOBJECT_METHOD_GT);
		break;

	case OPERATIVE_CODE_GE:
		BINOP(LOBJECT_METHOD_GE);
		break;

	case OPERATIVE_CODE_BAND:
		BINOP(LOBJECT_METHOD_BITWISE_AND);
		break;

	case OPERATIVE_CODE_BOR:
		BINOP(LOBJECT_METHOD_BITWISE_OR);
		break;

	default:
		return NULL;
	}
}

static struct generator_code *
peephole_rewrite_jmp_to_next(struct osic *osic,
                             struct generator_code *a)
{
	/*
	 *    JMP L1
	 * L1:
	 *    ...
	 * ->
	 * L1:
	 *    ...
	 */

	if (IS_JMP(a)) {
		if (a->next == a->arg[0]->label->code) {
			a = a->next;
			generator_delete_code(osic, a->prev);

			return a;
		}
	}

	return NULL;
}

static struct generator_code *
peephole_rewrite_jz_or_jnz_to_next(struct osic *osic,
                                   struct generator_code *a)
{
	/*
	 *    JZ/JNZ L1
	 * L1:
	 *    ...
	 * ->
	 *    POP
	 * L1:
	 *    g...
	 */

	if (IS_JZ(a) || IS_JNZ(a)) {
		if (a->next == a->arg[0]->label->code) {
			a->arg[0]->label->count -= 1;
			a->operative_code = OPERATIVE_CODE_POP;
			a->arg[0] = NULL;

			return a;
		}
	}

	return NULL;
}

static struct generator_code *
peephole_rewrite_dup_jz_to_dup_jz(struct osic *osic,
                                  struct generator_code *a)
{
	/*
	 *    DUP
	 *    JZ L1
	 *    ...
	 * L1:
	 *    DUP
	 *    JZ L2
	 *    ...
	 * ->
	 *    DUP
	 *    JZ L2
	 *    ...
	 * L1:
	 *    DUP
	 *    JZ L2
	 *    ...
	 */

	struct generator_code *b;

	if (IS_JZ(a) && IS_DUP(a->prev)) {
		b = a->arg[0]->label->code->next;
		if (IS_DUP(b) && IS_JZ(b->next)) {
			if (a->arg[0]->label == b->next->arg[0]->label) {
				return NULL;
			}

			a->arg[0]->label->count -= 1;
			b->next->arg[0]->label->count += 1;
			a->arg[0]->label = b->next->arg[0]->label;

			return a;
		}
	}

	return NULL;
}

static struct generator_code *
peephole_rewrite_const_jz(struct osic *osic,
                          struct generator_code *a)
{
	/*
	 *    CONST `true object'
	 *    JZ L1
	 *    ...
	 * L1:
	 *    ...
	 * ->
	 *    ...
	 * L1:
	 *    ...
	 */
	/*
	 *    CONST `false object'
	 *    JZ L1
	 *    ...
	 * L1:
	 *    ...
	 * ->
	 *    JMP L1
	 *    ...
	 * L1:
	 *    ...
	 */

	struct lobject *object;

	if (IS_JZ(a) && IS_CONST(a->prev)) {
		object = machine_get_const(osic, a->prev->arg[0]->value);
		if (lobject_boolean(osic, object) == osic->l_true) {
			generator_delete_code(osic, a->prev);
			a = a->next;
			generator_delete_code(osic, a->prev);
		} else {
			generator_delete_code(osic, a->prev);
			a->operative_code = OPERATIVE_CODE_JMP;
		}
		return a;
	}

	return NULL;
}

static struct generator_code *
peephole_rewrite_const_dup_jz(struct osic *osic,
                              struct generator_code *a)
{
	/*
	 *    CONST `true object'
	 *    DUP
	 *    JZ L1
	 *    ...
	 * L1:
	 *    ...
	 * ->
	 *    CONST `true object'
	 *    ...
	 * L1:
	 *    ...
	 */
	/*
	 *    CONST `false object'
	 *    DUP
	 *    JZ L1
	 *    ...
	 * L1:
	 *    ...
	 * ->
	 *    CONST `false object'
	 *    JMP L1
	 *    ...
	 * L1:
	 *    ...
	 */

	struct lobject *object;

	if (IS_JZ(a) && IS_DUP(a->prev) && IS_CONST(a->prev->prev)) {
		a = a->prev->prev;
		object = machine_get_const(osic, a->arg[0]->value);
		if (lobject_boolean(osic, object) == osic->l_true) {
			generator_delete_code(osic, a->next->next);
			generator_delete_code(osic, a->next);
		} else {
			generator_delete_code(osic, a->next);
			a->next->operative_code = OPERATIVE_CODE_JMP;
		}
		return a;
	}

	return NULL;
}

static struct generator_code *
peephole_rewrite_const_dup_jnz(struct osic *osic,
                               struct generator_code *a)
{
	/*
	 *    CONST `true object'
	 *    DUP
	 *    JNZ L1
	 *    ...
	 * L1:
	 *    ...
	 * ->
	 *    CONST `true object'
	 *    ...
	 * L1:
	 *    ...
	 */
	/*
	 *    CONST `false object'
	 *    DUP
	 *    JNZ L1
	 *    ...
	 * L1:
	 *    ...
	 * ->
	 *    CONST `false object'
	 *    JMP L1
	 *    ...
	 * L1:
	 *    ...
	 */

	struct lobject *object;

	if (IS_JNZ(a) && IS_DUP(a->prev) && IS_CONST(a->prev->prev)) {
		a = a->prev->prev;
		object = machine_get_const(osic, a->arg[0]->value);
		if (lobject_boolean(osic, object) == osic->l_true) {
			generator_delete_code(osic, a->next);
			a->next->operative_code = OPERATIVE_CODE_JMP;
		} else {
			generator_delete_code(osic, a->next->next);
			generator_delete_code(osic, a->next);
		}
		return a;
	}

	return NULL;
}

static struct generator_code *
peephole_rewrite_pop(struct osic *osic,
                     struct generator_code *a)
{
	/*
	 *    CONST/DUP
	 *    POP
	 *    ...
	 * ->
	 *    ...
	 */

	if (IS_POP(a) && (IS_CONST(a->prev) || IS_DUP(a->prev))) {
		generator_delete_code(osic, a->prev);
		a = a->next;
		generator_delete_code(osic, a->prev);

		return a;
	}

	return NULL;
}

static struct generator_code *
peephole_rewrite_load_load(struct osic *osic,
                           struct generator_code *a)
{
	/*
	 *    LOAD i
	 *    LOAD i
	 *    ...
	 * ->
	 *    LOAD i
	 *    DUP
	 *    ...
	 */

	if (IS_LOAD(a) && IS_LOAD(a->prev)) {
		if (a->arg[0]->value == a->prev->arg[0]->value &&
		    a->arg[1]->value == a->prev->arg[1]->value)
		{
			a->operative_code = OPERATIVE_CODE_DUP;
			a->arg[0] = NULL;
			a->arg[1] = NULL;

			return a->prev;
		}
	}

	return NULL;
}

static struct generator_code *
peephole_rewrite_store_load(struct osic *osic,
                            struct generator_code *a)
{
	/*
	 *    STORE i
	 *    LOAD i
	 *    ...
	 * ->
	 *    DUP
	 *    STORE ig
	 *    ...
	 */

	if (IS_LOAD(a) && IS_STORE(a->prev)) {
		if (a->arg[0]->value == a->prev->arg[0]->value &&
		    a->arg[1]->value == a->prev->arg[1]->value)
		{
			a->prev->operative_code = OPERATIVE_CODE_DUP;
			a->prev->arg[0] = NULL;
			a->operative_code = OPERATIVE_CODE_STORE;

			return a->prev;
		}
	}

	return NULL;
}

void
peephole_optimize(struct osic *osic)
{
	struct generator *gen;
	struct generator_code *a;
	struct generator_code *t;

	gen = osic->l_generator;
	a = gen->head;
	while (a) {
		if (IS_LABEL(a)) {
			/* remove no reference label,
			 * peephole should use backward order match pattern
			 */
			if (a->label && a->label->count == 0) {
				a = a->next;
				generator_delete_code(osic, a->prev);
			} else {
				a = a->next;
			}
			continue;
		}
		if (IS_NOP(a)) {
			a = a->next;
			generator_delete_code(osic, a->prev);
			continue;
		}

		if ((t = peephole_rewrite_unop(osic, a))) {
			a = t;
			continue;
		}

		if ((t = peephole_rewrite_binop(osic, a))) {
			a = t;
			continue;
		}

		if ((t = peephole_rewrite_load_load(osic, a))) {
			a = t;
			continue;
		}

		if ((t = peephole_rewrite_store_load(osic, a))) {
			a = t;
			continue;
		}

		if ((t = peephole_rewrite_jmp_to_next(osic, a))) {
			a = t;
			continue;
		}

		if ((t = peephole_rewrite_jz_or_jnz_to_next(osic, a))) {
			a = t;
			continue;
		}

		if ((t = peephole_rewrite_dup_jz_to_dup_jz(osic, a))) {
			a = t;
			continue;
		}

		if ((t = peephole_rewrite_const_jz(osic, a))) {
			a = t;
			continue;
		}

		if ((t = peephole_rewrite_const_dup_jz(osic, a))) {
			a = t;
			continue;
		}

		if ((t = peephole_rewrite_const_dup_jnz(osic, a))) {
			a = t;
			continue;
		}

		if ((t = peephole_rewrite_pop(osic, a))) {
			a = t;
			continue;
		}

		a = a->next;
	}
}
