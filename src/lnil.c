#include "osic.h"
#include "lnil.h"
#include "lstring.h"

static struct lobject *
lnil_method(struct osic *osic,
            struct lobject *self,
            int method, int argc, struct lobject *argv[])
{
	switch (method) {
	case LOBJECT_METHOD_STRING:
		return lstring_create(osic, "nil", 3);

	case LOBJECT_METHOD_BOOLEAN:
		return osic->l_false;

	default:
		return lobject_default(osic, self, method, argc, argv);
	}
}

void *
lnil_create(struct osic *osic)
{
	return lobject_create(osic, sizeof(struct lnil), lnil_method);
}
