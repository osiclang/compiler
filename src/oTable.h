#ifndef OSIC_OTABLE_H
#define OSIC_OTABLE_H

struct oobject;

struct otable {
	struct oobject object;

	int count;
	int length;

	void *items;
};

void *
otable_create(struct osic *osic);

struct otype *
otable_type_create(struct osic *osic);

#endif /* OSIC_OTABLE_H */
