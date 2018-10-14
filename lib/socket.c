#include "osic.h"
#include "larray.h"
#include "lmodule.h"
#include "lobject.h"
#include "lstring.h"
#include "linteger.h"
#include "lfunction.h"

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

static struct lobject *
socket_socket(struct osic *osic, struct lobject *self, int argc, struct lobject *argv[])
{
	int fd;
	int opt;
	int domain;
	int type;
	int protocol;

	if (argc > 3) {
		return lobject_error_argument(osic, "socket() too many arguments");
	}

	domain = 0;
	if (argc > 0) {
		if (!lobject_is_integer(osic, argv[0])) {
			return lobject_error_argument(osic, "socket() integer value required");
		}
		domain = linteger_to_long(osic, argv[0]);
	}

	type = 0;
	if (argc > 1) {
		if (!lobject_is_integer(osic, argv[1])) {
			return lobject_error_argument(osic, "socket() integer value required");
		}
		type = linteger_to_long(osic, argv[1]);
	}

	protocol = 0;
	if (argc > 2) {
		if (!lobject_is_integer(osic, argv[2])) {
			return lobject_error_argument(osic, "socket() integer value required");
		}
		protocol = linteger_to_long(osic, argv[2]);
	}

	fd = socket(domain, type, protocol);

	opt = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) != 0) {
		perror("setsockopt");
	}

	return linteger_create_from_long(osic, fd);
}

static struct lobject *
socket_bind(struct osic *osic, struct lobject *self, int argc, struct lobject *argv[])
{
	int fd;
	int port;
	struct sockaddr_in addr;

	if (argc != 2) {
		return lobject_error_argument(osic, "bind() required port");
	}

	port = linteger_to_long(osic, argv[1]);
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htons(INADDR_ANY);
	addr.sin_port = htons(port);

	fd = linteger_to_long(osic, argv[0]);
	if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		perror("bind");
	}

	return osic->l_nil;
}

static struct lobject *
socket_listen(struct osic *osic, struct lobject *self, int argc, struct lobject *argv[])
{
	int fd;
	int backlog;

	backlog = 16;
	if (argc > 1) {
		backlog = linteger_to_long(osic, argv[1]);
	}

	if (argc > 2) {
		return lobject_error_argument(osic, "listen() too many arguments");
	}

	fd = linteger_to_long(osic, argv[0]);
	if (listen(fd, backlog) == -1) {
		perror("listen");
	}

	return osic->l_nil;
}

static struct lobject *
socket_connect(struct osic *osic, struct lobject *self, int argc, struct lobject *argv[])
{
	int fd;
	int rc;
	long port;
	const char *host;

	struct sockaddr_in addr;

	if (argc != 3) {
		return lobject_error_argument(osic, "connect() require addr and port");
	}
	host = lstring_to_cstr(osic, argv[1]);
	port = linteger_to_long(osic, argv[2]);

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(host);

	fd = linteger_to_long(osic, argv[0]);
	rc = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
	if (rc == -1) {
		perror("connect");
	}

	return linteger_create_from_long(osic, rc);
}

static struct lobject *
socket_accept(struct osic *osic, struct lobject *a, int argc, struct lobject *argv[])
{
	int fd;
	int rc;
	struct sockaddr_in addr;
	char buffer[INET6_ADDRSTRLEN];
	socklen_t length = sizeof(addr);
	struct lobject *items[2];

	fd = linteger_to_long(osic, argv[0]);
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
		items[0] = lstring_create(osic, buffer, strlen(buffer));
		items[1] = linteger_create_from_long(osic, ntohs(addr.sin_port));
		items[1] = larray_create(osic, 2, items);
		items[0] = linteger_create_from_long(osic, rc);

		return larray_create(osic, 2, items);
	}

	items[0] = osic->l_empty_string;
	items[1] = linteger_create_from_long(osic, 0);
	items[1] = larray_create(osic, 2, items);
	items[0] = linteger_create_from_long(osic, rc);

	return larray_create(osic, 2, items);
}

static struct lobject *
socket_recv(struct osic *osic, struct lobject *a, int argc, struct lobject *argv[])
{
	int fd;
	int length;
	char buffer[4096];

	memset(buffer, 0, sizeof(buffer));
	fd = linteger_to_long(osic, argv[0]);
	length = recv(fd, buffer, 4096, 0);
	if (length > 0) {
		return lstring_create(osic, buffer, length);
	}

	return lstring_create(osic, NULL, 0);
}

static struct lobject *
socket_send(struct osic *osic, struct lobject *a, int argc, struct lobject *argv[])
{
	int fd;
	int rc;

	if (argc != 2) {
		return lobject_error_argument(osic, "send() require 2 arguments");
	}

	fd = linteger_to_long(osic, argv[0]);
	rc = send(fd,
	          lstring_buffer(osic, argv[1]),
	          lstring_length(osic, argv[1]),
	          0);
	if (rc < 0) {
		perror("send");
	}

	return linteger_create_from_long(osic, rc);
}

static struct lobject *
socket_shutdown(struct osic *osic, struct lobject *a, int argc, struct lobject *argv[])
{
	int fd;
	int rc;

	fd = linteger_to_long(osic, argv[0]);
	rc = shutdown(fd, SHUT_RDWR);

	return linteger_create_from_long(osic, rc);
}

struct lobject *
socket_module(struct osic *osic)
{
	struct lobject *name;
	struct lobject *module;

#ifdef WINDOWS
	WSADATA wsa;

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		printf("Failed. Error Code : %d", WSAGetLastError());

		return NULL;
	}
#endif

	module = lmodule_create(osic, lstring_create(osic, "socket", 6));

	name = lstring_create(osic, "socket", 6);
	lobject_set_attr(osic,
	                 module,
	                 name,
	                 lfunction_create(osic, name, NULL, socket_socket));

	name = lstring_create(osic, "bind", 4);
	lobject_set_attr(osic,
	                 module,
	                 name,
	                 lfunction_create(osic, name, NULL, socket_bind));

	name = lstring_create(osic, "listen", 6);
	lobject_set_attr(osic,
	                 module,
	                 name,
	                 lfunction_create(osic, name, NULL, socket_listen));

	name = lstring_create(osic, "connect", 7);
	lobject_set_attr(osic,
	                 module,
	                 name,
	                 lfunction_create(osic, name, NULL, socket_connect));

	name = lstring_create(osic, "accept", 6);
	lobject_set_attr(osic,
	                 module,
	                 name,
	                 lfunction_create(osic, name, NULL, socket_accept));

	name = lstring_create(osic, "send", 4);
	lobject_set_attr(osic,
	                 module,
	                 name,
	                 lfunction_create(osic, name, NULL, socket_send));

	name = lstring_create(osic, "recv", 4);
	lobject_set_attr(osic,
	                 module,
	                 name,
	                 lfunction_create(osic, name, NULL, socket_recv));

	name = lstring_create(osic, "shutdown", 8);
	lobject_set_attr(osic,
	                 module,
	                 name,
	                 lfunction_create(osic, name, NULL, socket_shutdown));

	name = lstring_create(osic, "AF_INET", 7);
	lobject_set_attr(osic,
	                 module,
	                 name,
	                 linteger_create_from_long(osic, AF_INET));

	name = lstring_create(osic, "SOCK_STREAM", 11);
	lobject_set_attr(osic,
	                 module,
	                 name,
	                 linteger_create_from_long(osic, SOCK_STREAM));

	return module;
}
