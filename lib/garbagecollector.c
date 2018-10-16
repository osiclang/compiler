#include "osic.h"
#include "garbagecollector.h"
#include "machine.h"

#include <limits.h>
#include <assert.h>
#include <string.h>

#define GC_STEP_RATIO 200
#define GC_STEP_THRESHOLD 2048

#define GC_FULL_RATIO 200
#define GC_FULL_THRESHOLD 65535

#define GC_MARK_MASK (uintptr_t)0x1UL
#define GC_SET_GRAY_MASK (uintptr_t)0x2UL
#define GC_ALL_MASK (GC_MARK_MASK | GC_SET_GRAY_MASK)

#define GC_SET_MASK(p,m) ((p) = (void *)((uintptr_t)(p) | (m)))
#define GC_CLR_MASK(p,m) ((p) = (void *)((uintptr_t)(p) & ~(m)))

#define GC_SET_MARK(a) (GC_SET_MASK((a)->l_next, GC_MARK_MASK))
#define GC_CLR_MARK(a) (GC_CLR_MASK((a)->l_next, GC_MARK_MASK))
#define GC_HAS_MARK(a) ((uintptr_t)(a)->l_next & GC_MARK_MASK)

#define GC_SET_GRAY(a) (GC_SET_MASK((a)->l_next, GC_SET_GRAY_MASK))
#define GC_CLR_GRAY(a) (GC_CLR_MASK((a)->l_next, GC_SET_GRAY_MASK))
#define GC_HAS_GRAY(a) ((uintptr_t)(a)->l_next & GC_SET_GRAY_MASK)

#define GC_GET_NEXT(a) ((void *)((uintptr_t)(a)->l_next & ~GC_ALL_MASK))

/*
 * Classic Tricolor Mark & Sweep GC Algorithm
 *
 * an object can have three state in GC_SWEEP_PHASE
 *
 * 	UNMARKED and UNGRAYED destroyable
 * 	UNMARKED and GRAYED   undestroyable
 * 	MARKED   and UNGRAYED undestroyable
 *
 * GRAY mask design to avoid destroy object in sweep,
 * because we can't mark and unmark an object while collector is sweeping.
 * so GRAYED object is still alive but don't need barrier any more.
 */

enum {
	GC_SCAN_PHASE,
	GC_MARK_PHASE,
	GC_SWEEP_PHASE
};

void *
collector_create(struct osic *osic)
{
	struct collector *collector;

	collector = osic_allocator_alloc(osic, sizeof(*collector));
	if (collector) {
		memset(collector, 0, sizeof(*collector));
		collector->phase = GC_SCAN_PHASE;
		collector->enabled = 1;

		collector->stacklen = 0;
		collector->stacktop = -1;

		collector->step_ratio = GC_STEP_RATIO;
		collector->step_threshold = GC_STEP_THRESHOLD;

		collector->full_ratio = GC_FULL_RATIO;
		collector->full_threshold = GC_FULL_THRESHOLD;
	}

	return collector;
}

void
collector_destroy(struct osic *osic, struct collector *collector)
{
	struct oobject *curr;
	struct oobject *next;

	curr = collector->object_list;
	while (curr) {
		next = GC_GET_NEXT(curr);
		oobject_destroy(osic, curr);
		curr = next;
	}

	osic_allocator_free(osic, collector->stack);
	osic_allocator_free(osic, collector);
}

int
collector_stack_is_empty(struct osic *osic)
{
	struct collector *collector;

	collector = osic->l_collector;

	return collector->stacktop < 0;
}

int
collector_stack_is_full(struct osic *osic)
{
	struct collector *collector;

	collector = osic->l_collector;

	return (collector->stacktop + 1) >= collector->stacklen;
}

void
collector_stack_push(struct osic *osic, struct oobject *object)
{
	struct collector *collector;

	collector = osic->l_collector;
	if (collector_stack_is_full(osic)) {
		int len;

		len = collector->stacklen ? collector->stacklen * 2 : 2;
		collector->stack = osic_allocator_realloc(osic,
		                              collector->stack,
		                              sizeof(struct oobject *) * len);
		if (!collector->stack) {
			return;
		}
		collector->stacklen = len;
	}

	collector->stack[++collector->stacktop] = object;
}

struct oobject *
collector_stack_pop(struct osic *osic)
{
	struct collector *collector;

	collector = osic->l_collector;
	if (!collector_stack_is_empty(osic)) {
		return collector->stack[collector->stacktop--];
	}

	return NULL;
}

void
osic_collector_enable(struct osic *osic)
{
	struct collector *collector;

	collector = osic->l_collector;
	collector->enabled = 1;
}

void
osic_collector_disable(struct osic *osic)
{
	struct collector *collector;

	collector = osic->l_collector;
	collector->enabled = 0;
}

int
osic_collector_enabled(struct osic *osic)
{
	struct collector *collector;

	collector = osic->l_collector;
	return collector->enabled;
}

void
osic_collector_mark(struct osic *osic, struct oobject *object)
{
	if (oobject_is_pointer(osic, object)) {
		if (!GC_HAS_MARK(object)) {
			GC_SET_MARK(object);
			GC_CLR_GRAY(object);
			collector_stack_push(osic, object);
		}
	}
}

void
collector_mark_children(struct osic *osic, struct oobject *object)
{
	GC_SET_MARK(object);
	GC_CLR_GRAY(object);
	oobject_method_call(osic, object, OOBJECT_METHOD_MARK, 0, NULL);
}

void
osic_collector_trace(struct osic *osic, struct oobject *object)
{
	struct collector *collector;

	collector = osic->l_collector;
	object->l_next = collector->object_list;
	collector->object_list = object;
	collector->live++;
	if (collector->sweeping && object->l_next == collector->sweeping) {
		collector->sweeping_prev = object;
	}
}

void
osic_collector_untrace(struct osic *osic, struct oobject *object)
{
	uintptr_t marked;
	struct collector *collector;
	struct oobject *curr;

	collector = osic->l_collector;
	curr = collector->object_list;

	if (curr == object) {
		collector->object_list = GC_GET_NEXT(curr);
	}

	while (curr && GC_GET_NEXT(curr) != object) {
		curr = GC_GET_NEXT(curr);
	}

	if (curr) {
		marked = GC_HAS_MARK(curr);
		curr->l_next = GC_GET_NEXT(object);
		if (marked) {
			GC_SET_MARK(curr);
		}
	}
}

/*
 * forward barrier
 */
void
osic_collector_barrier(struct osic *osic,
                        struct oobject *a,
                        struct oobject *b)
{
	struct collector *collector;

	collector = osic->l_collector;
	if (oobject_is_pointer(osic, b) && GC_HAS_MARK(a) && !GC_HAS_MARK(b)) {
		/*
		 * unmark object avoid repeat barrier
		 * gray object avoid sweep
		 */
		if (collector->phase == GC_SWEEP_PHASE) {
			GC_SET_GRAY(a);
			GC_CLR_MARK(a);
		}
		collector_stack_push(osic, b);
	}
}

/*
 * back barrier
 */
void
osic_collector_barrierback(struct osic *osic,
                            struct oobject *a,
                            struct oobject *b)
{
	struct collector *collector;

	collector = osic->l_collector;
	if (oobject_is_pointer(osic, b) && GC_HAS_MARK(a) && !GC_HAS_MARK(b)) {
		/*
		 * gray object avoid sweep
		 */
		if (collector->phase == GC_SWEEP_PHASE) {
			GC_SET_GRAY(a);
		}
		GC_CLR_MARK(a);
		collector_stack_push(osic, a);
	}
}

void
collector_scan_phase(struct osic *osic)
{
	int i;
	struct collector *collector;
	struct machine *machine;

	osic_mark_types(osic);
	osic_mark_errors(osic);
	osic_mark_strings(osic);

	machine = osic->l_machine;
	for (i = 0; i < machine->cpoollen; i++) {
		if (!machine->cpool[i]) {
			break;
		}
		osic_collector_mark(osic, machine->cpool[i]);
	}
	for (i = 0; i <= machine->sp; i++) {
		osic_collector_mark(osic, machine->stack[i]);
	}

	for (i = 0; i <= machine->fp; i++) {
		osic_collector_mark(osic,
		                     (struct oobject *)machine->frame[i]);
	}

	collector = osic->l_collector;
	collector->phase = GC_MARK_PHASE;
}

void
collector_mark_phase(struct osic *osic, long mark_max)
{
	long i;

	struct collector *collector;
	struct oobject *object;

	collector = osic->l_collector;
	for (i = 0; i < mark_max && !collector_stack_is_empty(osic); i++) {
		object = collector_stack_pop(osic);
		collector_mark_children(osic, object);
	}

	if (collector_stack_is_empty(osic)) {
		collector_scan_phase(osic);

		while (!collector_stack_is_empty(osic)) {
			object = collector_stack_pop(osic);
			collector_mark_children(osic, object);
		}

		collector->phase = GC_SWEEP_PHASE;
		collector->sweeping = collector->object_list;
	}
}

void
collector_sweep_phase(struct osic *osic, long swept_max)
{
	long swept_count;
	struct collector *collector;
	struct oobject *curr;
	struct oobject *prev;
	struct oobject *next;

	collector = osic->l_collector;
	prev = collector->sweeping_prev;
	curr = collector->sweeping;
	swept_count = 0;
	while (swept_count < swept_max && curr) {
		if (GC_HAS_MARK(curr) || GC_HAS_GRAY(curr)) {
			GC_CLR_MARK(curr);
			GC_CLR_GRAY(curr);
			prev = curr;
			curr = GC_GET_NEXT(curr);
		} else {
			next = NULL;
			if (prev) {
				prev->l_next = GC_GET_NEXT(curr);
				next = GC_GET_NEXT(curr);
			} else if (curr == collector->sweeping) {
				next = GC_GET_NEXT(curr);
				if (curr == collector->object_list) {
					collector->object_list = next;
				}
				collector->sweeping = next;
			}
			assert(next);

			oobject_destroy(osic, curr);
			curr = next;
			collector->live -= 1;
			swept_count += 1;
		}
		collector->sweeping_prev = prev;
		collector->sweeping = curr;
	}

	if (!curr) {
		collector->sweeping_prev = NULL;
		collector->sweeping = NULL;
		collector->phase = GC_SCAN_PHASE;
	}
}

void
collector_step(struct osic *osic, long step_max)
{
	struct collector *collector;

	collector = osic->l_collector;
	if (collector->phase == GC_SCAN_PHASE) {
		collector_scan_phase(osic);
	} else if (collector->phase == GC_MARK_PHASE) {
		collector_mark_phase(osic, step_max);
	} else {
		collector_sweep_phase(osic, step_max);
	}
	collector->step_threshold = collector->live + GC_STEP_THRESHOLD;
}

void
collector_full(struct osic *osic)
{
	long max;
	struct collector *collector;

	collector = osic->l_collector;
	if (collector->phase != GC_SCAN_PHASE) {
		do {
			collector_step(osic, LONG_MAX);
		} while (collector->phase != GC_SCAN_PHASE);
	}

	do {
		collector_step(osic, LONG_MAX);
	} while (collector->phase != GC_SCAN_PHASE);

	max = collector->live/100 * collector->full_ratio;
	if (max < GC_FULL_THRESHOLD) {
		max = GC_FULL_THRESHOLD;
	}

	collector->full_threshold = max;
}

void
collector_collect(struct osic *osic)
{
	long max;
	struct collector *collector;

	collector = osic->l_collector;
	if (collector->enabled) {
		if (collector->live >= collector->step_threshold) {
			max = GC_STEP_THRESHOLD/100 * collector->step_ratio;
			collector_step(osic, max);
		}

		if (collector->live >= collector->full_threshold) {
			collector_full(osic);
		}
	}
}
