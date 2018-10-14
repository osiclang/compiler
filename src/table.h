#ifndef OSIC_TABLE_H
#define OSIC_TABLE_H

#define TABLE_LOAD_FACTOR(n) ((n * 3 + 1) / 2)
#define TABLE_GROW_FACTOR(n) (n * 4)

struct slot {
	void *key;
	void *value;
};

typedef int (*table_cmp_t)(struct osic *, void *, void *);
typedef unsigned long (*table_hash_t)(struct osic *, void *);

unsigned long
table_size(struct osic *osic,
           unsigned long size);

/*
 * 0, replace exists key
 * 1, add a new key
 */
int
table_insert(struct osic *osic,
             void *key,
             void *value,
             struct slot *slots,
             unsigned long nslots,
             table_cmp_t cmp,
             table_hash_t hash);

void *
table_search(struct osic *osic,
             void *key,
             struct slot *slots,
             unsigned long nslots,
             table_cmp_t cmp,
             table_hash_t hash);

void *
table_delete(struct osic *osic,
             void *key,
             struct slot *slots,
             unsigned long nslots,
             table_cmp_t cmp,
             table_hash_t hash);

void
table_rehash(struct osic *osic,
             struct slot *oldslots,
             unsigned long noldslots,
             struct slot *newslots,
             unsigned long nnewslots,
             table_cmp_t cmp,
             table_hash_t hash);

#endif /* osic_TABLE_H */
