#ifndef OSIC_BUILTIN_H
#define OSIC_BUILTIN_H

struct osic;
struct lobject;

void
builtin_init(struct osic *osic);

struct lobject *
builtin_map(struct osic *osic,
            struct lobject *self,
            int argc, struct lobject *argv[]);

#endif /* osic_BUILTIN_H */
