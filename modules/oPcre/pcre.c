#include "osic.h"
#include "oArray.h"
#include "oString.h"
#include "oModule.h"

#include <pcre.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct opcre {
	struct oobject object;

	pcre *compiled;
	pcre_extra *extra;
};

struct oobject *
opcre_exec(struct osic *osic, struct oobject *self, int argc, struct oobject *argv[])
{
	int i;
	int rc;
	int offsets[30];
	const char *cstr;
	const char *match;
	struct opcre *opcre;
	struct oobject *array;

	opcre = (struct opcre *)self;

	cstr = ostring_to_cstr(osic, argv[0]);
	rc = pcre_exec(opcre->compiled, opcre->extra, cstr, strlen(cstr), 0, 0, offsets, 30);

	array = oarray_create(osic, 0, NULL);
	if (rc >= 0) {
		if (rc == 0) {
			rc = 30 / 3;
		}

		for (i = 0; i < rc; i++) {
			struct oobject *string;

			pcre_get_substring(cstr, offsets, rc, i, &match);
			string = ostring_create(osic, match, strlen(match));
			oarray_append(osic, array, 1, &string);
		}

		pcre_free_substring(match);
	}

	return array;
}

struct oobject *
opcre_get_attr(struct osic *osic, struct oobject *self, struct oobject *name)
{
        const char *cstr;

        cstr = ostring_to_cstr(osic, name);
        if (strcmp(cstr, "exec") == 0) {
                return ofunction_create(osic, name, self, opcre_exec);
        }

        return NULL;
}

struct oobject *
opcre_method(struct osic *osic,
             struct oobject *self,
             int method, int argc, struct oobject *argv[])
{
#define cast(a) ((struct opcre *)(a))        

        switch (method) {   
        case OOBJECT_METHOD_GET_ATTR:                    
                return opcre_get_attr(osic, self, argv[0]); 

        case OOBJECT_METHOD_DESTROY:
		pcre_free(cast(self)->compiled);
		if (cast(self)->extra) {
			pcre_free(cast(self)->extra);
		}
                return NULL;

        default:
                return oobject_default(osic, self, method, argc, argv);
        }
}

struct oobject *
opcre_compile(struct osic *osic, struct oobject *self, int argc, struct oobject *argv[])
{
	int offset;
	pcre *compiled;
	pcre_extra *extra;
	const char *error;
	const char *cstr;

	struct opcre *opcre;

	cstr = ostring_to_cstr(osic, argv[0]);

	compiled = pcre_compile(cstr, 0, &error, &offset, NULL);
	if (compiled == NULL) {
		printf("ERROR: Could not compile '%s': %s\n", cstr, error);

		return NULL;
	}

	extra = pcre_study(compiled, 0, &error);
	if (error != NULL) {
		printf("ERROR: Could not study '%s': %s\n", cstr, error);

		return NULL;
	}

	opcre = oobject_create(osic, sizeof(*opcre), opcre_method);
	opcre->compiled = compiled;
	opcre->extra = extra;

	return (struct oobject *)opcre;
}

struct oobject *
pcre_module(struct osic *osic)
{
        struct oobject *name;
        struct oobject *module;

        module = omodule_create(osic, ostring_create(osic, "pcre", 4));

        name = ostring_create(osic, "compile", 7);
        oobject_set_attr(osic,
                         module,
                         name,
                         ofunction_create(osic, name, NULL, opcre_compile));

        return module;
}
