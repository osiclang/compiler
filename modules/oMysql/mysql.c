#include "osic.h"
#include "oArray.h"
#include "oString.h"
#include "oModule.h"
#include "connection.h"
#include <stdio.h>
#include <mysql/mysql.h>

struct oobject *
omysql_connect(struct osic *osic, struct oobject *self, int argc, struct oobject *argv[])
{
	MYSQL *conn;
	char query[256];
	const char *host;
	const char *username;
	const char *password;
	const char *database;

	if (argc != 4) {
		return oobject_error_argument(osic, "connect() require host, username and password");
	}

	conn = mysql_init(NULL);
	if (!conn) {
		fprintf(stderr, "%s\n", mysql_error(conn));

		return NULL;
	}

	host = ostring_to_cstr(osic, argv[0]);
	username = ostring_to_cstr(osic, argv[1]);
	password = ostring_to_cstr(osic, argv[2]);
	database = ostring_to_cstr(osic, argv[3]);

	if (!mysql_real_connect(conn, host, username, password, NULL, 0, NULL, 0)) {
		fprintf(stderr, "%s\n", mysql_error(conn));
		mysql_close(conn);

		return NULL;
	}
	snprintf(query, sizeof(query), "USE %s", database);
	if (mysql_query(conn, query) != 0) {
		fprintf(stderr, "%s\n", mysql_error(conn));
		mysql_close(conn);

		return NULL;
	}

	return omysql_connection_create(osic, conn);
}

struct oobject *
mysql_module(struct osic *osic)
{
        struct oobject *name;
        struct oobject *module;

        module = omodule_create(osic, ostring_create(osic, "mysql", 5));

        name = ostring_create(osic, "connect", 7);
        oobject_set_attr(osic,
                         module,
                         name,
                         lfunction_create(osic, name, NULL, omysql_connect));

	printf("MySQL client version: %s\n", mysql_get_client_info());

	return module;
}
