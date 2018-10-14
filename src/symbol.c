#include "osic.h"
#include "arena.h"
#include "scope.h"
#include "symbol.h"

#include <string.h>

struct symbol *
symbol_make_symbol(struct osic *osic, const char *name, int type)
{
	struct symbol *symbol;

	symbol = arena_alloc(osic, osic->l_arena, sizeof(*symbol));
	if (!symbol) {
		return NULL;
	}
	memset(symbol, 0, sizeof(*symbol));
	strncpy(symbol->name, name, OSIC_NAME_MAX);
	symbol->type = type;

	return symbol;
}

struct symbol *
symbol_get_symbol(struct symbol *symbol, const char *name)
{
	for (; symbol; symbol = symbol->next) {
		if (strncmp(symbol->name, name, OSIC_NAME_MAX) == 0) {
			return symbol;
		}
	}

	return NULL;
}
