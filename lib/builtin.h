#ifndef OSIC_BUILTIN_H
#define OSIC_BUILTIN_H

struct osic;
struct oobject;

void
builtin_init(struct osic *osic);

struct oobject *
builtin_map(struct osic *osic,
            struct oobject *self,
            int argc, struct oobject *argv[]);

#endif /* osic_BUILTIN_H */
