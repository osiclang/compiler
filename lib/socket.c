#include "osic.h"
#include "oArray.h"
#include "oModule.h"
#include "oObject.h"
#include "oString.h"
#include "oInteger.h"
#include "oFunction.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifdef WINDOWS
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x501
#endif
#include <Winsock2.h>
#include <Windows.h>
#include <Ws2tcpip.h>
typedef int socklen_t;
#else
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46
#endif

#ifndef SHUT_RD
#define SHUT_RD 0
#endif

#ifndef SHUT_WR
#define SHUT_WR 1
#endif

#ifndef SHUT_RDWR
#define SHUT_RDWR 2
#endif

static struct oobject *
socket_socket(struct osic *osic, struct oobject *self, int argc, struct oobject *argv[])
{
	int fd;
	int opt;
	int domain;
	int type;
	int protocol;

	if (argc > 3) {
		return oobject_error_argument(osic, "socket() too many arguments");
	}

	domain = 0;
	if (argc > 0) {
		if (!oobject_is_integer(osic, argv[0])) {
			return oobject_error_argument(osic, "socket() integer value required");
		}
		domain = ointeger_to_long(osic, argv[0]);
	}

	type = 0;
	if (argc > 1) {
		if (!oobject_is_integer(osic, argv[1])) {
			return oobject_error_argument(osic, "socket() integer value required");
		}
		type = ointeger_to_long(osic, argv[1]);
	}

	protocol = 0;
	if (argc > 2) {
		if (!oobject_is_integer(osic, argv[2])) {
			return oobject_error_argument(osic, "socket() integer value required");
		}
		protocol = ointeger_to_long(osic, argv[2]);
	}

	fd = socket(domain, type, protocol);

	opt = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) != 0) {
		perror("setsockopt");
	}

	return ointeger_create_from_long(osic, fd);
}

static struct oobject *
socket_bind(struct osic *osic, struct oobject *self, int argc, struct oobject *argv[])
{
	int fd;
	int port;
	struct sockaddr_in addr;

	if (argc != 2) {
		return oobject_error_argument(osic, "bind() required port");
	}

	port = ointeger_to_long(osic, argv[1]);
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htons(INADDR_ANY);
	addr.sin_port = htons(port);

	fd = ointeger_to_long(osic, argv[0]);
	if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		perror("bind");
	}

	return osic->l_nil;
}

static struct oobject *
socket_listen(struct osic *osic, struct oobject *self, int argc, struct oobject *argv[])
{
	int fd;
	int backlog;

	backlog = 16;
	if (argc > 1) {
		backlog = ointeger_to_long(osic, argv[1]);
	}

	if (argc > 2) {
		return oobject_error_argument(osic, "listen() too many arguments");
	}

	fd = ointeger_to_long(osic, argv[0]);
	if (listen(fd, backlog) == -1) {
		perror("listen");
	}

	return osic->l_nil;
}

static struct oobject *
socket_connect(struct osic *osic, struct oobject *self, int argc, struct oobject *argv[])
{
	int fd;
	int rc;
	long port;
	const char *host;

	struct sockaddr_in addr;

	if (argc != 3) {
		return oobject_error_argument(osic, "connect() require addr and port");
	}
	host = ostring_to_cstr(osic, argv[1]);
	port = ointeger_to_long(osic, argv[2]);

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(host);

	fd = ointeger_to_long(osic, argv[0]);
	rc = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
	if (rc == -1) {
		perror("connect");
	}

	return ointeger_create_from_long(osic, rc);
}

static struct oobject *
socket_accept(struct osic *osic, struct oobject *a, int argc, struct oobject *argv[])
{
	int fd;
	int rc;
	struct sockaddr_in addr;
	char buffer[INET6_ADDRSTRLEN];
	socklen_t length = sizeof(addr);
	struct oobject *items[2];

	fd = ointeger_to_long(osic, argv[0]);
	rc = accept(fd, (struct sockaddr *)&addr, &length);
	if (rc == -1) {
		perror("accept");

	}

	if (!getnameinfo((struct sockaddr *)&addr,
	                 sizeof(addr),
	                 buffer,
	                 sizeof(buffer),
	                 NULL,
	                 0,
	                 NI_NUMERICHOST))
	{
		items[0] = ostring_create(osic, buffer, strlen(buffer));
		items[1] = ointeger_create_from_long(osic, ntohs(addr.sin_port));
		items[1] = oarray_create(osic, 2, items);
		items[0] = ointeger_create_from_long(osic, rc);

		return oarray_create(osic, 2, items);
	}

	items[0] = osic->l_empty_string;
	items[1] = ointeger_create_from_long(osic, 0);
	items[1] = oarray_create(osic, 2, items);
	items[0] = ointeger_create_from_long(osic, rc);

	return oarray_create(osic, 2, items);
}

static struct oobject *
socket_recv(struct osic *osic, struct oobject *a, int argc, struct oobject *argv[])
{
	int fd;
	int length;
	char buffer[4096];

	memset(buffer, 0, sizeof(buffer));
	fd = ointeger_to_long(osic, argv[0]);
	length = recv(fd, buffer, 4096, 0);
	if (length > 0) {
		return ostring_create(osic, buffer, length);
	}

	return ostring_create(osic, NULL, 0);
}

static struct oobject *
socket_send(struct osic *osic, struct oobject *a, int argc, struct oobject *argv[])
{
	int fd;
	int rc;

	if (argc != 2) {
		return oobject_error_argument(osic, "send() require 2 arguments");
	}

	fd = ointeger_to_long(osic, argv[0]);
	rc = send(fd,
	          ostring_buffer(osic, argv[1]),
	          ostring_length(osic, argv[1]),
	          0);
	if (rc < 0) {
		perror("send");
	}

	return ointeger_create_from_long(osic, rc);
}

static struct oobject *
socket_shutdown(struct osic *osic, struct oobject *a, int argc, struct oobject *argv[])
{
	int fd;
	int rc;

	fd = ointeger_to_long(osic, argv[0]);
	rc = shutdown(fd, SHUT_RDWR);

	return ointeger_create_from_long(osic, rc);
}

struct oobject *
socket_module(struct osic *osic)
{
	struct oobject *name;
	struct oobject *module;

#ifdef WINDOWS
	WSADATA wsa;

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		printf("Failed. Error Code : %d", WSAGetLastError());

		return NULL;
	}
#endif

	module = omodule_create(osic, ostring_create(osic, "socket", 6));

	name = ostring_create(osic, "socket", 6);
	oobject_set_attr(osic,
	                 module,
	                 name,
	                 ofunction_create(osic, name, NULL, socket_socket));

	name = ostring_create(osic, "bind", 4);
	oobject_set_attr(osic,
	                 module,
	                 name,
	                 ofunction_create(osic, name, NULL, socket_bind));

	name = ostring_create(osic, "listen", 6);
	oobject_set_attr(osic,
	                 module,
	                 name,
	                 ofunction_create(osic, name, NULL, socket_listen));

	name = ostring_create(osic, "connect", 7);
	oobject_set_attr(osic,
	                 module,
	                 name,
	                 ofunction_create(osic, name, NULL, socket_connect));

	name = ostring_create(osic, "accept", 6);
	oobject_set_attr(osic,
	                 module,
	                 name,
	                 ofunction_create(osic, name, NULL, socket_accept));

	name = ostring_create(osic, "send", 4);
	oobject_set_attr(osic,
	                 module,
	                 name,
	                 ofunction_create(osic, name, NULL, socket_send));

	name = ostring_create(osic, "recv", 4);
	oobject_set_attr(osic,
	                 module,
	                 name,
	                 ofunction_create(osic, name, NULL, socket_recv));

	name = ostring_create(osic, "shutdown", 8);
	oobject_set_attr(osic,
	                 module,
	                 name,
	                 ofunction_create(osic, name, NULL, socket_shutdown));

	name = ostring_create(osic, "AF_INET", 7);
	oobject_set_attr(osic,
	                 module,
	                 name,
	                 ointeger_create_from_long(osic, AF_INET));

	name = ostring_create(osic, "SOCK_STREAM", 11);
	oobject_set_attr(osic,
	                 module,
	                 name,
	                 ointeger_create_from_long(osic, SOCK_STREAM));

	return module;
}
