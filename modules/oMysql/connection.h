#ifndef OSIC_MYSQL_CONNECTION_SOCKET_H
#define OSIC_MYSQL_CONNECTION_SOCKET_H

#include "oObject.h"

#include <mysql.h>

struct omysql_connection {
        struct oobject object;

        MYSQL *mysql;
};

void *
omysql_connection_create(struct osic *osic, MYSQL *mysql);

#endif /* OSIC_MYSQL_CONNECTION_SOCKET_H*/
