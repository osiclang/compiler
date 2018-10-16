#include "osic.h"
#include "oNil.h"
#include "oString.h"

static struct oobject *
onil_method(struct osic *osic,
            struct oobject *self,
            int method, int argc, struct oobject *argv[])
{
	switch (method) {
	case OOBJECT_METHOD_STRING:
		return ostring_create(osic, "nil", 3);

	case OOBJECT_METHOD_BOOLEAN:
		return osic->l_false;

	default:
		return oobject_default(osic, self, method, argc, argv);
	}
}

void *
onil_create(struct osic *osic)
{
	return oobject_create(osic, sizeof(struct onil), onil_method);
}
