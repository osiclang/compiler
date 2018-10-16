#include "osic.h"
#include "oString.h"
#include "oSentinel.h"

static struct oobject *
osentinel_method(struct osic *osic,
                 struct oobject *self,
                 int method, int argc, struct oobject *argv[])
{
	switch (method) {
	case OOBJECT_METHOD_STRING:
		return ostring_create(osic, "sentinel", 8);

	default:
		return oobject_default(osic, self, method, argc, argv);
	}
}

void *
osentinel_create(struct osic *osic)
{
	struct osentinel *self;

	self = oobject_create(osic, sizeof(*self), osentinel_method);

	return self;
}
