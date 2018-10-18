#ifndef OSIC_MYSQL_CURSOR_H
#define OSIC_MYSQL_CURSOR_H

#include "oObject.h"

#include <mysql.h>

struct omysql_cursor {
	struct oobject object;

	struct oobject *description;
	MYSQL_RES *meta;
	MYSQL_STMT *stmt;
};

void *
omysql_cursor_create(struct osic *osic, MYSQL_STMT *stmt);

#endif /* OSIC_MYSQL_CURSOR_H */
