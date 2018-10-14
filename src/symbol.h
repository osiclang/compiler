#ifndef OSIC_SYMBOL_H
#define OSIC_SYMBOL_H

struct osic;
struct syntax;

enum {
	SYMBOL_LOCAL,
	SYMBOL_GLOBAL,
	SYMBOL_EXCEPTION
};

struct symbol {
	int type;

	int cpool; /* const pool index */
	int level; /* level == 0: current frame, level > 0: upframe */
	int local; /* frame->locals index */
	char name[OSIC_NAME_MAX];

	struct symbol *next;
	struct syntax *accessor_list;
};

struct symbol *
symbol_make_symbol(struct osic *osic, const char *name, int type);

struct symbol *
symbol_get_symbol(struct symbol *symbol, const char *name);

#endif /* osic_SYMBOL_H */
