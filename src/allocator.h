#ifndef OSIC_ALLOCATOR_H
#define OSIC_ALLOCATOR_H

/*
 * there is three level memory management in osic
 *
 * 1. `osic_allocator_alloc' and `osic_allocator_free'
 *    basic alloc and free interface, ensured aligned pointer
 *
 * 2. `arena_alloc'
 *    internal allocator for lexer and parser no free action needed
 *
 * 3. `osic_collector_trace' and `osic_collector_mark'
 *    garbage collector for osic object system
 */

/*
 * allocator has two policy of alloc,
 * 1, (allocation size >> SIZE_SHIFT) <= ALLOCATOR_POOL_SIZE
 *    use fix size memory pool, pool[] element is double linked list
 * 2, (allocation size >> SIZE_SHIFT) > ALLOCATOR_POOL_SIZE
 *    use memalign_malloc dynamic size align allocation or system malloc,
 *    if malloc return align pointer, use `make USE_MALLOC=1', default is 0.
 */
struct osic;
struct mpool;

#ifndef ALLOCATOR_POOL_SIZE
#define ALLOCATOR_POOL_SIZE 32
#endif

struct allocator {
	struct mpool *pool[ALLOCATOR_POOL_SIZE];
};

void *
allocator_create(struct osic *osic);

void
allocator_destroy(struct osic *osic, struct allocator *mem);

void *
allocator_alloc(struct osic *osic, long size);

void
allocator_free(struct osic *osic, void *ptr);

void *
allocator_realloc(struct osic *osic, void *ptr, long size);

#endif /* osic_ALLOCATOR_H */
