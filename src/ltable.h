#ifndef OSIC_LTABLE_H
#define OSIC_LTABLE_H

struct lobject;

struct ltable {
	struct lobject object;

	int count;
	int length;

	void *items;
};

void *
ltable_create(struct osic *osic);

struct ltype *
ltable_type_create(struct osic *osic);

#endif /* osic_LTABLE_H */
