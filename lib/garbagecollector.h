#ifndef OSIC_GC_H
#define OSIC_GC_H

struct collector {
	int phase;
	int enabled;

	long live; /* number of live objects */

	long step_ratio; /* ratio to perform action */
	long step_threshold;

	long full_ratio; /* ratio to compute threshold */
	long full_threshold;

	int stacklen;
	int stacktop;
	struct oobject **stack;

	struct oobject *object_list;
	struct oobject *sweeping_prev;
	struct oobject *sweeping;
};

void *
collector_create(struct osic *osic);

void
collector_destroy(struct osic *osic, struct collector *collector);

void
collector_step(struct osic *osic, long step_max);

void
collector_full(struct osic *osic);

void
collector_collect(struct osic *osic);

#endif /* OSIC_GC_H */
