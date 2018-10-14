#ifndef OSIC_SCOPE_H
#define OSIC_SCOPE_H

struct osic;
struct symbol;

enum {
	SCOPE_CLASS,
	SCOPE_BLOCK,
	SCOPE_DEFINE,
	SCOPE_MODULE
};

struct scope {
	int type;
	struct scope *parent;

	struct symbol *symbol;
};

int
scope_enter(struct osic *osic, int type);

int
scope_leave(struct osic *osic);

struct symbol *
scope_add_symbol(struct osic *osic,
                 struct scope *scope,
                 const char *name,
                 int type);

struct symbol *
scope_get_symbol(struct osic *osic, struct scope *scope, char *name);

#endif /* osic_SCOPE_H */
