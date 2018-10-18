#include "osic.h"
#include "oArray.h"
#include "oString.h"
#include "oModule.h"

#include "cursor.h"
#include "connection.h"

#include <stdio.h>
#include <string.h>
#include <mysql.h>

struct oobject *           
omysql_connection_cursor(struct osic *osic, struct oobject *self, int argc, struct oobject *argv[])
{
	MYSQL *mysql;
	MYSQL_STMT *stmt;

	mysql = ((struct omysql_connection *)self)->mysql;

	stmt = mysql_stmt_init(mysql);

	return omysql_cursor_create(osic, stmt);
}

struct oobject *
omysql_connection_get_attr(struct osic *osic, struct oobject *self, struct oobject *name)
{
        const char *cstr;

        cstr = ostring_to_cstr(osic, name);
        if (strcmp(cstr, "cursor") == 0) {
                return ofunction_create(osic, name, self, omysql_connection_cursor);
        }

	return NULL;
}

struct oobject *
omysql_connection_method(struct osic *osic,
                         struct oobject *self,
                         int method, int argc, struct oobject *argv[])
{
#define cast(a) ((struct omysql_connection *)(a))

	switch (method) {
        case OOBJECT_METHOD_GET_ATTR:
                return omysql_connection_get_attr(osic, self, argv[0]);

        case OOBJECT_METHOD_DESTROY:
		mysql_close(cast(self)->mysql);
		return NULL;

	default:
		return oobject_default(osic, self, method, argc, argv);
	}
}

void *
omysql_connection_create(struct osic *osic, MYSQL *mysql)
{
	struct omysql_connection *self;

	self = oobject_create(osic, sizeof(*self), omysql_connection_method);
	if (self) {
		self->mysql = mysql;
	}

	return self;
}
