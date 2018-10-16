#include "osic.h"
#include "oNumber.h"
#include "oString.h"
#include "oInteger.h"

#include <stdio.h>
#include <limits.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define SMALLINT_MAX (long)(ULONG_MAX >> 2)
#define SMALLINT_MIN (long)(-SMALLINT_MAX)

void *
ointeger_create(struct osic *osic, int digits);

void *
ointeger_create_object_from_long(struct osic *osic, long value);

void *
ointeger_create_object_from_integer(struct osic *osic,
                                    struct oobject *integer);

static void
normalize(struct osic *osic, struct ointeger *a)
{
	a->ndigits = extend_length(a->length, a->digits);;

	if (a->ndigits == 0) {
		a->sign = 1;
	}
}

int
ointeger_cmp(struct osic *osic, struct oobject *a, struct oobject *b)
{
	struct ointeger *ia;
	struct ointeger *ib;

	if (!oobject_is_pointer(osic, a) && !oobject_is_pointer(osic, b)) {
		long la;
		long lb;

		la = ointeger_to_long(osic, a);
		lb = ointeger_to_long(osic, b);

		if (la > lb) {
			return 1;
		}

		if (la < lb) {
			return -1;
		}

		return 0;
	}

	assert(oobject_is_pointer(osic, a) && oobject_is_pointer(osic, b));

	ia = (struct ointeger *)a;
	ib = (struct ointeger *)b;

	if (ia->sign == 1 && ib->sign == 0) {
		return 1;
	}

	if (ia->sign == 0 && ib->sign == 1) {
		return -1;
	}

	if (ia->ndigits > ib->ndigits) {
		return 1;
	}

	if (ia->ndigits < ib->ndigits) {
		return -1;
	}

	if (ia->sign) {
		return extend_cmp(ia->ndigits, ia->digits, ib->digits);
	} else {
		return extend_cmp(ia->ndigits, ib->digits, ia->digits);
	}

	return 0;
}

/*
 * CERT Coding Standard INT32-C Integer Overflow Check Algorithms
 */
static struct oobject *
ointeger_add(struct osic *osic, struct oobject *a, struct oobject *b)
{
	int ndigits;
	struct ointeger *ia;
	struct ointeger *ib;
	struct ointeger *ic;
	unsigned long carry;

	if (!oobject_is_pointer(osic, a) && !oobject_is_pointer(osic, b)) {
		long la;
		long lb;

		la = ointeger_to_long(osic, a);
		lb = ointeger_to_long(osic, b);

		if ((lb > 0 && la > (LONG_MAX - lb)) ||
		    (lb < 0 && la < (LONG_MIN - lb)))
		{
			goto promot;
		}

		return ointeger_create_from_long(osic, la + lb);
	}
promot:
	ia = ointeger_create_object_from_integer(osic, a);
	if (!ia) {
		return NULL;
	}

	ib = ointeger_create_object_from_integer(osic, b);
	if (!ib) {
		return NULL;
	}

	if (ia->sign == ib->sign) {
		if (ia->ndigits > ib->ndigits) {
			ndigits = ia->ndigits + 1;
		} else {
			ndigits = ib->ndigits + 1;
		}
		ic = ointeger_create(osic, ndigits);

		carry = extend_add(ic->digits,
		                   ia->ndigits,
		                   ia->digits,
		                   ib->ndigits,
		                   ib->digits,
		                   0);
		ic->digits[ic->length - 1] = (extend_t)(carry % EXTEND_BASE);
		ic->sign = ia->sign;
	} else if (ointeger_cmp(osic,
	                        (struct oobject *)ia,
	                        (struct oobject *)ib) > 0)
	{
		ic = ointeger_create(osic, ia->ndigits);
		extend_sub(ic->digits,
		           ia->ndigits,
		           ia->digits,
		           ib->ndigits,
		           ib->digits,
		           0);
		ic->sign = ia->sign;
	} else {
		ic = ointeger_create(osic, ib->ndigits);
		extend_sub(ic->digits,
		           ib->ndigits,
		           ib->digits,
		           ia->ndigits,
		           ia->digits,
		           0);
		ic->sign = ib->sign;
	}

	normalize(osic, ic);
	return (struct oobject *)ic;
}

static struct oobject *
ointeger_sub(struct osic *osic, struct oobject *a, struct oobject *b)
{
	int ndigits;
	struct ointeger *ia;
	struct ointeger *ib;
	struct ointeger *ic;
	unsigned long carry;

	if (!oobject_is_pointer(osic, a) && !oobject_is_pointer(osic, b)) {
		long la;
		long lb;

		la = ointeger_to_long(osic, a);
		lb = ointeger_to_long(osic, b);

		if ((lb > 0 && la < LONG_MIN + lb) ||
		    (lb < 0 && la > LONG_MAX + lb))
		{
			goto promot;
		}

		return ointeger_create_from_long(osic, la - lb);
	}

promot:
	ia = ointeger_create_object_from_integer(osic, a);
	if (!ia) {
		return NULL;
	}

	ib = ointeger_create_object_from_integer(osic, b);
	if (!ib) {
		return NULL;
	}

	if (ia->sign != ib->sign) {
		if (ia->ndigits > ib->ndigits) {
			ndigits = ia->ndigits + 1;
		} else {
			ndigits = ib->ndigits + 1;
		}
		ic = ointeger_create(osic, ndigits);

		carry = extend_add(ic->digits,
		                   ia->ndigits,
		                   ia->digits,
		                   ib->ndigits,
		                   ib->digits,
		                   0);
		ic->digits[ic->length - 1] = (extend_t)(carry & EXTEND_BASE);
		ic->sign = ia->sign;
	} else if (ointeger_cmp(osic,
	                        (struct oobject *)ia,
	                        (struct oobject *)ib) > 0)
	{
		ic = ointeger_create(osic, ia->ndigits);
		extend_sub(ic->digits,
		           ia->ndigits,
		           ia->digits,
		           ib->ndigits,
		           ib->digits,
		           0);
		ic->sign = ia->sign;
	} else {
		ic = ointeger_create(osic, ib->ndigits);
		extend_sub(ic->digits,
		           ib->ndigits,
		           ib->digits,
		           ia->ndigits,
		           ia->digits,
		           0);
		ic->sign = -ib->sign;
	}

	normalize(osic, ic);
	return (struct oobject *)ic;
}

static struct oobject *
ointeger_mul(struct osic *osic, struct oobject *a, struct oobject *b)
{
	struct ointeger *ia;
	struct ointeger *ib;
	struct ointeger *ic;

	if (!oobject_is_pointer(osic, a) && !oobject_is_pointer(osic, b)) {
		long la;
		long lb;

		la = ointeger_to_long(osic, a);
		lb = ointeger_to_long(osic, b);

		if (la > 0) {  /* la is positive */
			if (lb > 0) {  /* la and lb are positive */
				if (la > (LONG_MAX / lb)) {
					goto promot;
				}
			} else { /* la positive, lb nonpositive */
				if (lb < (LONG_MIN / la)) {
					goto promot;
				}
			} /* la positive, lb nonpositive */
		} else { /* la is nonpositive */
			if (lb > 0) { /* la is nonpositive, lb is positive */
				if (la < (LONG_MIN / lb)) {
					goto promot;
				}
			} else { /* la and lb are nonpositive */
				if ( (la != 0) && (lb < (LONG_MAX / la))) {
					goto promot;
				}
			} /* End if la and lb are nonpositive */
		} /* End if la is nonpositive */

		return ointeger_create_from_long(osic, la * lb);
	}

promot:
	ia = ointeger_create_object_from_integer(osic, a);
	if (!ia) {
		return NULL;
	}

	ib = ointeger_create_object_from_integer(osic, b);
	if (!ib) {
		return NULL;
	}

	ic = ointeger_create(osic, ia->ndigits + ib->ndigits);
	extend_mul(ic->digits,
	           ia->ndigits,
	           ia->digits,
	           ib->ndigits,
	           ib->digits);
	ic->sign = ia->sign == ib->sign ? 1 : -1;

	normalize(osic, ic);
	return (struct oobject *)ic;
}

static struct oobject *
ointeger_div(struct osic *osic, struct oobject *a, struct oobject *b)
{
	struct ointeger *ia;
	struct ointeger *ib;
	struct ointeger *ic;
	struct ointeger *id;
	struct ointeger *ie;

	if (!oobject_is_pointer(osic, a) && !oobject_is_pointer(osic, b)) {
		long la;
		long lb;

		la = ointeger_to_long(osic, a);
		lb = ointeger_to_long(osic, b);

		if (lb == 0) {
			const char *fmt;

			fmt = "divide by zero '%@/0'";
			return oobject_error_arithmetic(osic, fmt, a);
		}

		if ((la == LONG_MIN) && (lb == -1)) {
			goto promot;
		}

		return ointeger_create_from_long(osic, la / lb);
	}

promot:
	ia = ointeger_create_object_from_integer(osic, a);
	if (!ia) {
		return NULL;
	}

	ib = ointeger_create_object_from_integer(osic, b);
	if (!ib) {
		return NULL;
	}

	ic = ointeger_create(osic, ia->ndigits);
	if (!ic) {
		return NULL;
	}
	id = ointeger_create(osic, ib->ndigits);
	if (!id) {
		return NULL;
	}
	ie = ointeger_create(osic, ia->ndigits + ib->ndigits + 2);
	if (!ie) {
		return NULL;
	}
	extend_div(ic->digits,
	           ia->ndigits,
	           ia->digits,
	           ib->ndigits,
	           ib->digits,
	           id->digits,
	           ie->digits);
	ic->sign = ia->sign == ib->sign ? 1 : -1;

	normalize(osic, ic);
	return (struct oobject *)ic;
}

static struct oobject *
ointeger_mod(struct osic *osic, struct oobject *a, struct oobject *b)
{
	struct ointeger *ia;
	struct ointeger *ib;
	struct ointeger *ic;
	struct ointeger *id;
	struct ointeger *ie;

	if (!oobject_is_pointer(osic, a) && !oobject_is_pointer(osic, b)) {
		long la;
		long lb;

		la = ointeger_to_long(osic, a);
		lb = ointeger_to_long(osic, b);

		if (lb == 0) {
			const char *fmt;

			fmt = "divide by zero '%@/0'";
			return oobject_error_arithmetic(osic, fmt, a);
		}

		if ((la == LONG_MIN) && (lb == -1)) {
			goto promot;
		}

		return ointeger_create_from_long(osic, la % lb);
	}
promot:
	ia = ointeger_create_object_from_integer(osic, a);
	if (!ia) {
		return NULL;
	}

	ib = ointeger_create_object_from_integer(osic, b);
	if (!ib) {
		return NULL;
	}

	ic = ointeger_create(osic, ia->ndigits);
	if (!ic) {
		return NULL;
	}
	id = ointeger_create(osic, ib->ndigits);
	if (!id) {
		return NULL;
	}
	ie = ointeger_create(osic, ia->ndigits + ib->ndigits + 2);
	if (!ie) {
		return NULL;
	}
	extend_div(ic->digits,
	           ia->ndigits,
	           ia->digits,
	           ib->ndigits,
	           ib->digits,
	           id->digits,
	           ie->digits);
	ic->sign = ia->sign == ib->sign ? 1 : -1;

	normalize(osic, ic);
	return (struct oobject *)id;
}

static struct oobject *
ointeger_neg(struct osic *osic, struct oobject *a)
{
	struct ointeger *ia;
	struct ointeger *ic;

	if (!oobject_is_pointer(osic, a)) {
		long la;

		la = ointeger_to_long(osic, a);

		return ointeger_create_from_long(osic, -la);
	}

	ia = (struct ointeger *)a;
	ic = ointeger_create(osic, ia->ndigits);
	if (!ic) {
		return NULL;
	}
	memcpy(ic->digits, ia->digits, ia->ndigits * sizeof(extend_t));
	ic->sign = ia->sign ? 0 : 1;
	ic->ndigits = ia->ndigits;

	return (struct oobject *)ic;
}

static struct oobject *
ointeger_shl(struct osic *osic, struct oobject *a, struct oobject *b)
{
	long s;
	int ndigits;
	struct ointeger *ia;
	struct ointeger *ic;

	if (!oobject_is_pointer(osic, a) && !oobject_is_pointer(osic, b)) {
		long la;
		long lb;

		la = ointeger_to_long(osic, a);
		lb = ointeger_to_long(osic, b);

		if (lb < 0) {
			const char *fmt;

			fmt = "negative shift '%@ << %@'";
			return oobject_error_arithmetic(osic, fmt, a, b);
		}

		if ((lb >= (long)sizeof(la) * 8) || (la > (LONG_MAX >> lb))) {
			goto promot;
		}

		return ointeger_create_from_long(osic, la << lb);
	}
promot:
	ia = ointeger_create_object_from_integer(osic, a);
	if (!ia) {
		return NULL;
	}

	s = (ointeger_to_long(osic, b) + EXTEND_BITS-1) & ~(EXTEND_BITS-1);
	ndigits = ia->ndigits + (int)s / EXTEND_BITS;
	ic = ointeger_create(osic, ndigits);
	if (!ic) {
		return NULL;
	}
	extend_shl(ic->length, ic->digits, ia->ndigits, ia->digits, (int)s, 0);
	ic->sign = ia->sign;

	normalize(osic, ic);
	return (struct oobject *)ic;
}

static struct oobject *
ointeger_shr(struct osic *osic, struct oobject *a, struct oobject *b)
{
	int s;
	struct ointeger *ia;
	struct ointeger *ic;

	if (!oobject_is_pointer(osic, a) && !oobject_is_pointer(osic, b)) {
		long la;
		long lb;

		la = ointeger_to_long(osic, a);
		lb = ointeger_to_long(osic, b);

		if (lb < 0) {
			const char *fmt;

			fmt = "negative shift '%@ >> %@'";
			return oobject_error_arithmetic(osic, fmt, a, b);
		}

		return ointeger_create_from_long(osic, la >> lb);
	}

	ia = ointeger_create_object_from_integer(osic, a);
	if (!ia) {
		return NULL;
	}

	s = (int)ointeger_to_long(osic, b);
	if (s >= 8 * ia->ndigits) {
		return ointeger_create(osic, 0);
	}

	ic = ointeger_create(osic, ia->ndigits - s/EXTEND_BITS);
	if (!ic) {
		return NULL;
	}
	extend_shr(ic->length, ic->digits, ia->ndigits, ia->digits, s, 0);
	ic->sign = ia->sign;

	normalize(osic, ic);
	return (struct oobject *)ic;
}

static struct oobject *
ointeger_bitwise_not(struct osic *osic, struct oobject *a)
{
	extend_t one[1];
	struct ointeger *ia;
	struct ointeger *ic;
	unsigned long carry;

	if (!oobject_is_pointer(osic, a)) {
		long la;

		la = ointeger_to_long(osic, a);
		return ointeger_create_from_long(osic, ~la);
	}

	ia = (struct ointeger *)a;
	ic = ointeger_create(osic, ia->ndigits + 1);
	if (!ic) {
		return NULL;
	}

	one[0] = 1;
	carry = extend_add(ic->digits, ia->ndigits, ia->digits, 1, one, 0);
	ic->digits[ic->length - 1] = (extend_t)(carry % EXTEND_BASE);
	ic->sign = ia->sign ? 0 : 1;

	normalize(osic, ic);
	return (struct oobject *)ic;
}

static struct oobject *
ointeger_bitwise_and(struct osic *osic, struct oobject *a, struct oobject *b)
{
	int i;
	int ndigits;
	struct ointeger *ia;
	struct ointeger *ib;
	struct ointeger *ic;

	if (!oobject_is_pointer(osic, a) && !oobject_is_pointer(osic, b)) {
		long la;
		long lb;

		la = ointeger_to_long(osic, a);
		lb = ointeger_to_long(osic, b);

		return ointeger_create_from_long(osic, la & lb);
	}

	ia = ointeger_create_object_from_integer(osic, a);
	if (!ia) {
		return NULL;
	}

	ib = ointeger_create_object_from_integer(osic, b);
	if (!ib) {
		return NULL;
	}

	if (ia->ndigits < ib->ndigits) {
		ndigits = ia->ndigits;
	} else {
		ndigits = ib->ndigits;
	}
	ic = ointeger_create(osic, ndigits);
	if (!ic) {
		return NULL;
	}
	ic->sign = ia->sign;

	for (i = 0; i < ic->ndigits; i++) {
		ic->digits[i] = ia->digits[i] & ib->digits[i];
	}

	normalize(osic, ic);

	return (struct oobject *)ic;
}

static struct oobject *
ointeger_bitwise_xor(struct osic *osic, struct oobject *a, struct oobject *b)
{
	int i;
	int min;
	int max;
	struct ointeger *ia;
	struct ointeger *ib;
	struct ointeger *ic;
	struct ointeger *x;

	if (!oobject_is_pointer(osic, a) && !oobject_is_pointer(osic, b)) {
		long la;
		long lb;

		la = ointeger_to_long(osic, a);
		lb = ointeger_to_long(osic, b);

		return ointeger_create_from_long(osic, la ^ lb);
	}

	ia = ointeger_create_object_from_integer(osic, a);
	if (!ia) {
		return NULL;
	}

	ib = ointeger_create_object_from_integer(osic, b);
	if (!ib) {
		return NULL;
	}

	if (ia->ndigits > ib->ndigits) {
		max = ia->ndigits;
		min = ib->ndigits;
		x = ia;
	} else {
		min = ia->ndigits;
		max = ib->ndigits;
		x = ib;
	}
	ic = ointeger_create(osic, max);
	if (!ic) {
		return NULL;
	}
	ic->sign = ia->sign;

	for (i = 0; i < min; i++) {
		ic->digits[i] = ia->digits[i] ^ ib->digits[i];
	}

	for (; i < max; i++) {
		ic->digits[i] = x->digits[i] ^ 0;
	}

	normalize(osic, ic);
	return (struct oobject *)ic;
}

static struct oobject *
ointeger_bitwise_or(struct osic *osic, struct oobject *a, struct oobject *b)
{
	int i;
	int min;
	int max;
	struct ointeger *ia;
	struct ointeger *ib;
	struct ointeger *ic;
	struct ointeger *x;

	if (!oobject_is_pointer(osic, a) && !oobject_is_pointer(osic, b)) {
		long la;
		long lb;

		la = ointeger_to_long(osic, a);
		lb = ointeger_to_long(osic, b);

		return ointeger_create_from_long(osic, la | lb);
	}

	ia = ointeger_create_object_from_integer(osic, a);
	if (!ia) {
		return NULL;
	}

	ib = ointeger_create_object_from_integer(osic, b);
	if (!ib) {
		return NULL;
	}

	if (ia->ndigits > ib->ndigits) {
		x = ia;
		min = ib->ndigits;
		max = ia->ndigits;
	} else {
		x = ib;
		max = ib->ndigits;
		min = ia->ndigits;
	}
	ic = ointeger_create(osic, max);
	if (!ic) {
		return NULL;
	}
	ic->sign = ia->sign;

	for (i = 0; i < min; i++) {
		ic->digits[i] = ia->digits[i] | ib->digits[i];
	}

	for (; i < max; i++) {
		ic->digits[i] = x->digits[i];
	}

	normalize(osic, ic);
	return (struct oobject *)ic;
}

static struct oobject *
ointeger_string(struct osic *osic, struct oobject *self)
{
	char buffer[40960];
	char *p;
	int size;
	struct ointeger *a;
	struct ointeger *integer;

	if (!oobject_is_pointer(osic, self)) {
		snprintf(buffer,
			 sizeof(buffer),
			 "%ld",
			 ointeger_to_long(osic, self));
	} else {
		integer = (struct ointeger *)self;
		if (integer->ndigits == 0) {
			return ostring_create(osic, "0", 1);
		}

		if (integer->sign == 0) {
			buffer[0] = '-';
			p = buffer + 1;
			size = sizeof(buffer) - 1;
		} else {
			p = buffer;
			size = sizeof(buffer);
		}
		a = ointeger_create(osic, integer->ndigits);
		memcpy(a->digits,
		       integer->digits,
		       integer->ndigits * sizeof(extend_t));
		a->ndigits = integer->ndigits;
		extend_to_str(a->ndigits, a->digits, p, size, 10);
	}
	buffer[sizeof(buffer) - 1] = '\0';

	return ostring_create(osic, buffer, strlen(buffer));
}

struct oobject *
ointeger_method(struct osic *osic,
                struct oobject *self,
                int method, int argc, struct oobject *argv[])
{

#define binop(op) do {                                                         \
	if (oobject_is_integer(osic, argv[0])) {                              \
		return ointeger_ ## op (osic, self, argv[0]);                 \
	}                                                                      \
	if (oobject_is_number(osic, argv[0])) {                               \
		struct oobject *number;                                        \
		if (oobject_is_pointer(osic, self)) {                         \
			const char *cstr;                                      \
			struct oobject *string;                                \
			string = ointeger_string(osic, self);                 \
			cstr = ostring_to_cstr(osic, string);                 \
			number = onumber_create_from_cstr(osic, cstr);        \
		} else {                                                       \
			long value;                                            \
			value = ointeger_to_long(osic, self);                 \
			number = onumber_create_from_long(osic, value);       \
		}                                                              \
		if (!number) {                                                 \
			return NULL;                                           \
		}                                                              \
		return oobject_method_call(osic, number, method, argc, argv); \
	}                                                                      \
	return oobject_default(osic, self, method, argc, argv);               \
} while (0)

#define cmpop(op) do {                                                         \
	if (oobject_is_integer(osic, argv[0])) {                              \
		struct oobject *a = self;                                      \
		struct oobject *b = argv[0];                                   \
		if (oobject_is_pointer(osic, a) ||                            \
		    oobject_is_pointer(osic, b))                              \
		{                                                              \
			a = ointeger_create_object_from_integer(osic, a);     \
			if (!a) {                                              \
				return NULL;                                   \
			}                                                      \
			b = ointeger_create_object_from_integer(osic, b);     \
			if (!b) {                                              \
				return NULL;                                   \
			}                                                      \
		}                                                              \
		if (ointeger_cmp(osic, a, b) op 0) {                          \
			return osic->l_true;                                  \
		}                                                              \
		return osic->l_false;                                         \
	}                                                                      \
	if (oobject_is_number(osic, argv[0])) {                               \
		struct oobject *number;                                        \
		if (oobject_is_pointer(osic, self)) {                         \
			const char *cstr;                                      \
			struct oobject *string;                                \
			string = ointeger_string(osic, self);                 \
			cstr = ostring_to_cstr(osic, string);                 \
			number = onumber_create_from_cstr(osic, cstr);        \
		} else {                                                       \
			long value;                                            \
			value = ointeger_to_long(osic, self);                 \
			number = onumber_create_from_long(osic, value);       \
		}                                                              \
		if (!number) {                                                 \
			return NULL;                                           \
		}                                                              \
		return oobject_method_call(osic, number, method, argc, argv); \
	}                                                                      \
	return oobject_default(osic, self, method, argc, argv);               \
} while (0)

	switch (method) {
	case OOBJECT_METHOD_ADD:
		binop(add);

	case OOBJECT_METHOD_SUB:
		binop(sub);

	case OOBJECT_METHOD_MUL:
		binop(mul);

	case OOBJECT_METHOD_DIV:
		binop(div);

	case OOBJECT_METHOD_MOD:
		binop(mod);

	case OOBJECT_METHOD_POS:
		return self;

	case OOBJECT_METHOD_NEG:
		return ointeger_neg(osic, self);

	case OOBJECT_METHOD_SHL:
		binop(shl);

	case OOBJECT_METHOD_SHR:
		binop(shr);

	case OOBJECT_METHOD_LT:
		cmpop(<);

	case OOBJECT_METHOD_LE:
		cmpop(<=);

	case OOBJECT_METHOD_EQ:
		cmpop(==);

	case OOBJECT_METHOD_NE:
		cmpop(!=);

	case OOBJECT_METHOD_GE:
		cmpop(>=);

	case OOBJECT_METHOD_GT:
		cmpop(>);

	case OOBJECT_METHOD_BITWISE_NOT:
		return ointeger_bitwise_not(osic, self);

	case OOBJECT_METHOD_BITWISE_AND:
		binop(bitwise_and);

	case OOBJECT_METHOD_BITWISE_XOR:
		binop(bitwise_xor);

	case OOBJECT_METHOD_BITWISE_OR:
		binop(bitwise_or);

	case OOBJECT_METHOD_HASH:
		return self;

	case OOBJECT_METHOD_NUMBER:
		return self;

	case OOBJECT_METHOD_INTEGER:
		return self;

	case OOBJECT_METHOD_BOOLEAN:
		if (ointeger_to_long(osic, self)) {
			return osic->l_true;
		}
		return osic->l_false;

	case OOBJECT_METHOD_STRING:
		return ointeger_string(osic, self);

	default:
		return oobject_default(osic, self, method, argc, argv);
	}
}

long
ointeger_to_long(struct osic *osic, struct oobject *object)
{
	uintptr_t sign;
	long value;

	if (oobject_is_pointer(osic, object)) {
		unsigned long uvalue;
		struct ointeger *integer;

		integer = (struct ointeger *)object;
		uvalue = extend_to_long(integer->ndigits, integer->digits);
		if (uvalue > LONG_MAX) {
			return LONG_MAX;
		}

		value = uvalue;
		if (integer->sign == -1) {
			value = -value;
		}
	} else {
		sign = ((uintptr_t)object) & 0x2;
		value = ((uintptr_t)object) >> 2;
		if (sign) {
			value = -value;
		}
	}

	return value;
}

void *
ointeger_create(struct osic *osic, int digits)
{
	size_t size;
	struct ointeger *self;

	size = digits * sizeof(extend_t);

	self = oobject_create(osic, sizeof(*self) + size, ointeger_method);
	if (self) {
		self->sign = 1;
		self->length = digits;
		self->ndigits = 1;
	}

	return self;
}

void *
ointeger_create_object_from_long(struct osic *osic, long value)
{
	struct ointeger *self;

	self = ointeger_create(osic, sizeof(long));
	if (self) {
		if (value < 0) {
			self->sign = 0;
			value = -value;
		}
		extend_from_long(self->length, self->digits, value);
		normalize(osic, self);
	}

	return self;
}

void *
ointeger_create_object_from_integer(struct osic *osic,
                                    struct oobject *integer)
{
	long value;

	if (oobject_is_pointer(osic, integer)) {
		return integer;
	}
	value = ointeger_to_long(osic, integer);

	return ointeger_create_object_from_long(osic, value);
}

void *
ointeger_create_from_long(struct osic *osic, long value)
{
	struct ointeger *integer;

	/* create smallint if possible */
	if (value > SMALLINT_MIN && value < SMALLINT_MAX) {
		/*
		 * signed value shifting is undefined behavior in C
		 * so change negative value to postivie and pack 1 sign bit
		 * never use value directly, two's complement is not necessary
		 */
		int sign; /* 0, positive, 1 negative */

		sign = 0;
		if (value < 0) {
			sign = 0x2;
			value = -value;
		}

		integer = (void *)(((uintptr_t)value << 2) | sign | 0x1);

		return integer;
	}

	return ointeger_create_object_from_long(osic, value);
}

void *
ointeger_create_from_cstr(struct osic *osic, const char *cstr)
{
	long value;
	struct ointeger *self;

	value = strtol(cstr, NULL, 0);
	if (value != LONG_MAX && value != LONG_MIN) {
		return ointeger_create_from_long(osic, value);
	}

	self = ointeger_create(osic, (int)strlen(cstr));
	if (self) {
		if (cstr[0] == '0') {
			if (cstr[1] == 'x' || cstr[1] == 'X') {
				extend_from_str(self->length,
				                self->digits,
				                cstr + 2,
				                16,
				                NULL);
			} else {
				extend_from_str(self->length,
				                self->digits,
				                cstr + 1,
				                8,
				                NULL);
			}
		} else {
			extend_from_str(self->length,
			                self->digits,
			                cstr,
			                10,
			                NULL);
		}
		normalize(osic, self);
	}

	return self;
}

static struct oobject *
ointeger_type_method(struct osic *osic,
                     struct oobject *self,
                     int method, int argc, struct oobject *argv[])
{
	switch (method) {
	case OOBJECT_METHOD_CALL: {
		long value;

		if (argc < 1) {
			return ointeger_create_from_long(osic, 0);
		}

		value = 0;
		if (oobject_is_number(osic, argv[0])) {
			value = (long)onumber_to_double(osic, argv[0]);
		}

		if (oobject_is_string(osic, argv[0])) {
			const char *cstr;

			cstr = ostring_to_cstr(osic, argv[0]);
			value = strtol(cstr, NULL, 10);
		}

		return ointeger_create_from_long(osic, value);
	}

	case OOBJECT_METHOD_CALLABLE:
		return osic->l_true;

	default:
		return oobject_default(osic, self, method, argc, argv);
	}
}

struct otype *
ointeger_type_create(struct osic *osic)
{
	struct otype *type;

	type = otype_create(osic,
	                    "integer",
	                    ointeger_method,
	                    ointeger_type_method);
	if (type) {
		osic_add_global(osic, "integer", type);
	}

	return type;
}

