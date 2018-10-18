#include "osic.h"
#include "cursor.h"

#include "oArray.h"
#include "oString.h"
#include "oModule.h"
#include "oInteger.h"

#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <mysql.h>

struct oobject *
omysql_cursor_execute(struct osic *osic, struct oobject *self, int argc, struct oobject *argv[])
{
	int i;
	int count;
	MYSQL_RES *meta;
	MYSQL_STMT *stmt;
	MYSQL_BIND *bind;
	const char *query;
	my_bool max_length;
	struct omysql_cursor *cursor;

	if (argc < 1 || !oobject_is_string(osic, argv[0])) {
		return oobject_error_argument(osic, "execute() require 1 query string");
	}
	query = ostring_to_cstr(osic, argv[0]);

	cursor = (struct omysql_cursor *)self;
	stmt = cursor->stmt;
	if (mysql_stmt_prepare(stmt, query, strlen(query))) {
		fprintf(stderr, "mysql_stmt_prepare() %s\n", mysql_stmt_error(stmt));

		return NULL;
	}

	count = mysql_stmt_param_count(stmt);
	if (count > argc - 1) {
		return oobject_error_argument(osic, "execute() arguments not match parameter");
	}

	max_length = 1;
	mysql_stmt_attr_set(stmt, STMT_ATTR_UPDATE_MAX_LENGTH, &max_length);

	meta = mysql_stmt_result_metadata(stmt);
	cursor->description = osic->l_nil;
	if (meta) {
		int nfields;
		MYSQL_FIELD *fields;
		struct oobject *field;
		struct oobject *description;

		fields = mysql_fetch_fields(meta);
		nfields = mysql_num_fields(meta);
		description = oarray_create(osic, 0, NULL);
		for (i = 0; i < nfields; i++) {
			struct oobject *items[2];

			items[0] = ostring_create(osic, fields[i].name, strlen(fields[i].name));
			items[1] = ointeger_create_from_long(osic, fields[i].max_length);
			field = oarray_create(osic, 2, items);
			oarray_append(osic, description, 1, &field);

		}
		cursor->meta = meta;
		cursor->description = description;
	}

	if (count) {
		bind = osic_allocator_alloc(osic, sizeof(MYSQL_BIND) * count);
		memset(bind, 0, sizeof(MYSQL_BIND) * count);

		for (i = 0; i < count; i++) {
			struct oobject *object;

			object = argv[i + 1];

			if (oobject_is_integer(osic, object)) {
				void *buffer;
				long value;

				buffer = osic_allocator_alloc(osic, sizeof(long));
				value = ointeger_to_long(osic, object);
				memcpy(buffer, &value, sizeof(value));
				
				bind[i].buffer_type = MYSQL_TYPE_LONG;
				bind[i].buffer = buffer;
				bind[i].is_unsigned = 0;
				bind[i].is_null = 0;
			} else if (oobject_is_string(osic, object)) {
				bind[i].buffer_type = MYSQL_TYPE_STRING;
				bind[i].buffer = ostring_buffer(osic, object);
				bind[i].buffer_length = ostring_length(osic, object);
				bind[i].is_unsigned = 0;
				bind[i].is_null = 0;
			}
		}

		if (mysql_stmt_bind_param(stmt, bind) != 0) {
			fprintf(stderr, "mysql_stmt_bind_param() %s\n", mysql_stmt_error(stmt));

			return NULL;
		}
	}

	if (mysql_stmt_execute(stmt) != 0) {
		fprintf(stderr, "mysql_stmt_execute() %s\n", mysql_stmt_error(stmt));

		return NULL;
	}

	if (count) {
		for (i = 0; i < count; i++) {
			if (bind[i].buffer_type == MYSQL_TYPE_LONG) {
				osic_allocator_free(osic, bind[i].buffer);
			}
		}
		osic_allocator_free(osic, bind);
	}

	if (mysql_stmt_store_result(stmt)) {
		fprintf(stderr, "mysql_stmt_bind_result() %s\n", mysql_stmt_error(stmt));

		return NULL;
	}

	return osic->l_nil;
}

struct oobject *
omysql_cursor_fetchmany(struct osic *osic, struct oobject *self, int argc, struct oobject *argv[])
{
	int i;
	int nfields;
	long many;
	MYSQL_RES *meta;
	MYSQL_STMT *stmt;
	MYSQL_BIND *bind;
	MYSQL_FIELD *fields;
	struct oobject *row;
	struct oobject *results;
	struct oobject **items;
	struct omysql_cursor *cursor;

	many = LONG_MAX;
	if (argc) {
		many = ointeger_to_long(osic, argv[0]);
	}

	cursor = (struct omysql_cursor *)self;
	stmt = cursor->stmt;
	if (!cursor->meta) {
		return oobject_error_type(osic, "no result");
	}

	meta = cursor->meta;
	fields = mysql_fetch_fields(meta);
	nfields = mysql_num_fields(meta);
	if (!nfields) {
		return oobject_error_type(osic, "no result");
	}
	bind = osic_allocator_alloc(osic, sizeof(MYSQL_BIND) * nfields);
	memset(bind, 0, sizeof(MYSQL_BIND) * nfields);

	for (i = 0; i < nfields; i++) {
		if (fields[i].type == MYSQL_TYPE_LONG) {
			bind[i].buffer_type = MYSQL_TYPE_LONG;
			bind[i].buffer = osic_allocator_alloc(osic, sizeof(long));
			bind[i].is_unsigned = 0;
			bind[i].is_null = 0;
		} else if (fields[i].type == MYSQL_TYPE_STRING ||
		           fields[i].type == MYSQL_TYPE_VAR_STRING)
		{
			bind[i].buffer_type = fields[i].type;
			bind[i].buffer = osic_allocator_alloc(osic, fields[i].max_length);
			bind[i].buffer_length = fields[i].max_length;
			bind[i].length = osic_allocator_alloc(osic, sizeof(unsigned long));
			bind[i].is_unsigned = 0;
			bind[i].is_null = 0;
		}
	}

	if (mysql_stmt_bind_result(stmt, bind) != 0) {
		fprintf(stderr, "mysql_stmt_bind_result() %s\n", mysql_stmt_error(stmt));

		return NULL;
	}

	results = oarray_create(osic, 0, NULL);
	items = osic_allocator_alloc(osic, sizeof(struct oobject *) * nfields);
	while (many--) {
		if (mysql_stmt_fetch(stmt) != 0) {
			mysql_stmt_free_result(stmt);
			mysql_free_result(meta);
			cursor->meta = NULL;

			break;
		}
		for (i = 0; i < nfields; i++) {
			struct oobject *object;
			if (bind[i].buffer_type == MYSQL_TYPE_LONG) {
				object = ointeger_create_from_long(osic, *(long *)bind[i].buffer);
			} else if (bind[i].buffer_type == MYSQL_TYPE_STRING ||
			           bind[i].buffer_type == MYSQL_TYPE_VAR_STRING)
			{
				object = ostring_create(osic, bind[i].buffer, *bind[i].length);
			}
			items[i] = object;
		}
		row = oarray_create(osic, nfields, items);
		oarray_append(osic, results, 1, &row);
	}
	osic_allocator_free(osic, items);

	for (i = 0; i < nfields; i++) {
		osic_allocator_free(osic, bind[i].buffer);
		osic_allocator_free(osic, bind[i].length);
	}
	osic_allocator_free(osic, bind);

	return results;
}

struct oobject *
omysql_cursor_fetchone(struct osic *osic, struct oobject *self, int argc, struct oobject *argv[])
{
	struct oobject *one;

	one = ointeger_create_from_long(osic, 1);

	return omysql_cursor_fetchmany(osic, self, 1, &one);
}

struct oobject *
omysql_cursor_fetchall(struct osic *osic, struct oobject *self, int argc, struct oobject *argv[])
{
	return omysql_cursor_fetchmany(osic, self, 0, NULL);
}

struct oobject *
omysql_cursor_get_attr(struct osic *osic,
                           struct oobject *self,
                           struct oobject *name)
{
        const char *cstr;

        cstr = ostring_to_cstr(osic, name);
        if (strcmp(cstr, "execute") == 0) {
                return ofunction_create(osic, name, self, omysql_cursor_execute);
        }

        if (strcmp(cstr, "fetchone") == 0) {
                return ofunction_create(osic, name, self, omysql_cursor_fetchone);
        }

        if (strcmp(cstr, "fetchmany") == 0) {
                return ofunction_create(osic, name, self, omysql_cursor_fetchmany);
        }

        if (strcmp(cstr, "fetchall") == 0) {
                return ofunction_create(osic, name, self, omysql_cursor_fetchall);
        }

        if (strcmp(cstr, "description") == 0) {
		return ((struct omysql_cursor *)self)->description;
        }

	return NULL;
}

struct oobject *
omysql_cursor_method(struct osic *osic,
                     struct oobject *self,
                     int method, int argc, struct oobject *argv[])
{
#define cast(a) ((struct omysql_cursor *)(a))

	switch (method) {
        case OOBJECT_METHOD_GET_ATTR:
                return omysql_cursor_get_attr(osic, self, argv[0]);

        case OOBJECT_METHOD_MARK:
		if (cast(self)->description) {
			oobject_mark(osic, cast(self)->description);
		}
		return NULL;

        case OOBJECT_METHOD_DESTROY:
		mysql_stmt_free_result(cast(self)->stmt);
		mysql_free_result(cast(self)->meta);
		mysql_stmt_close(cast(self)->stmt);
		return NULL;

	default:
		return oobject_default(osic, self, method, argc, argv);
	}
}

void *
omysql_cursor_create(struct osic *osic, MYSQL_STMT *stmt)
{
	struct omysql_cursor *self;

	self = oobject_create(osic, sizeof(*self), omysql_cursor_method);
	if (self) {
		self->stmt = stmt;
	}

	return self;
}
