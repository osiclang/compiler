#include "osic.h"
#include "lstring.h"
#include "lsentinel.h"

static struct lobject *
lsentinel_method(struct osic *osic,
                 struct lobject *self,
                 int method, int argc, struct lobject *argv[])
{
	switch (method) {
	case LOBJECT_METHOD_STRING:
		return lstring_create(osic, "sentinel", 8);

	default:
		return lobject_default(osic, self, method, argc, argv);
	}
}

void *
lsentinel_create(struct osic *osic)
{
	struct lsentinel *self;

	self = lobject_create(osic, sizeof(*self), lsentinel_method);

	return self;
}
