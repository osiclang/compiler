#include "osic.h"
#include "oArray.h"
#include "oString.h"
#include "lib/builtin.h"
#include "shell.h"

#ifdef MODULE_OS
#include "lib/os.h"
#endif

#ifdef MODULE_SOCKET
#include "lib/socket.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
main(int argc, char *argv[])
{
	int i;
	struct osic *osic;
	struct oobject *objects;

	osic = osic_create();
	if (!osic) {
		printf("create osic fail\n");
		return 0;
	}
	builtin_init(osic);

	/* internal module */
#ifdef MODULE_OS
	oobject_set_item(osic,
	                 osic->l_modules,
	                 ostring_create(osic, "os", 2),
	                 os_module(osic));
#endif

#ifdef MODULE_SOCKET
	oobject_set_item(osic,
	                 osic->l_modules,
	                 ostring_create(osic, "socket", 6),
	                 socket_module(osic));
#endif

	if (argc < 2) {
		shell(osic);
	} else {
		if (!osic_input_set_file(osic, argv[1])) {
			fprintf(stderr, "open '%s' file fail\n", argv[1]);
            osic_destroy(osic);
			exit(1);
		}

		objects = oarray_create(osic, 0, NULL);
		for (i = 1; i < argc; i++) {
			struct oobject *value;

			value = ostring_create(osic, argv[i], strlen(argv[i]));
			oarray_append(osic, objects, 1, &value);
		}
		osic_add_global(osic, "argv", objects);

		if (!osic_compile(osic)) {
			fprintf(stderr, "osic: syntax error\n");
            osic_destroy(osic);
			exit(1);
		}

		osic_machine_reset(osic);
		osic_machine_execute(osic);
	}

    osic_destroy(osic);

	return 0;
}
