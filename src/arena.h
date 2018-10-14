#ifndef OSIC_ARENA_H
#define OSIC_ARENA_H

struct arena {
	char *limit;
	char *avail;

	char **blocks;
	int iblocks;
	int nblocks;
};

struct arena *
arena_create(struct osic *osic);

void
arena_destroy(struct osic *osic, struct arena *arena);

void *
arena_alloc(struct osic *osic, struct arena *arena, long bytes);

#endif /* osic_ARENA_H */
