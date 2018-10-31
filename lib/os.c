#include "osic.h"
#include "oArray.h"
#include "oModule.h"
#include "oString.h"
#include "oInteger.h"

#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#ifdef WINDOWS
#include <windows.h>
#include <winsock2.h>
#else
#include <sys/select.h>
#endif

static struct oobject *
os_open(struct osic *osic, struct oobject *self, int argc, struct oobject *argv[])
{
	int fd;
	int oflag;
	int mode;

#if WINDOWS
	mode = S_IREAD | S_IWRITE;
#else
	mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
#endif
	if (argc == 3) {
		mode = ointeger_to_long(osic, argv[2]);
	}

	if (argc >= 2) {
		oflag = ointeger_to_long(osic, argv[1]);
	} else {
		oflag = O_RDONLY;
	}
        
	if (oflag & O_CREAT) {
		fd = open(ostring_to_cstr(osic, argv[0]), oflag, mode);
	} else {
		fd = open(ostring_to_cstr(osic, argv[0]), oflag);
	}
        
	if (fd == -1) {
		return oobject_error_type(osic, "open '%@' fail: %s", argv[0], strerror(errno));
	}

	return ointeger_create_from_long(osic, fd);
}


static struct oobject *
os_read(struct osic *osic, struct oobject *self, int argc, struct oobject *argv[])
{
	int fd;
	long pos;
	long end;
	size_t size;
	struct stat stat;
	struct oobject *string;

	fd = ointeger_to_long(osic, argv[0]);
	if (fd < 0) {
		return oobject_error_type(osic, "can't read '%@'", argv[0]);
	}

	pos = lseek(fd, 0, SEEK_CUR);
	if (pos < 0) {
		return oobject_error_type(osic, "can't read '%@'", argv[0]);
	}

	if (fstat(fd, &stat) < 0) {
		return oobject_error_type(osic, "can't read '%@'", argv[0]);
	}

	end = stat.st_size;
	string = ostring_create(osic, NULL, end - pos);
	size = read(fd, ostring_buffer(osic, string), end - pos);
	if (size < (size_t)(end - pos)) {
		return ostring_create(osic,
		                      ostring_buffer(osic, string),
		                      size);
	}

	return string;
}

static struct oobject *
os_write(struct osic *osic, struct oobject *self, int argc, struct oobject *argv[])
{
	int fd;
	size_t size;

	fd = ointeger_to_long(osic, argv[0]);
	size = write(fd,
	             ostring_to_cstr(osic, argv[1]),
	             ostring_length(osic, argv[1]));
       
	if (size) {
		return osic->l_true;
	}
	return osic->l_false;
}

static struct oobject *
os_close(struct osic *osic, struct oobject *self, int argc, struct oobject *argv[])
{
	int fd;

	fd = ointeger_to_long(osic, argv[0]);
	close(fd);

	return osic->l_nil;
}

#ifndef WINDOWS
static struct oobject *
os_fcntl(struct osic *osic, struct oobject *self, int argc, struct oobject *argv[])
{
	int fd;
	int cmd;
	int flags;
	int value;

	if (argc < 2 || argc > 3) {
		return oobject_error_argument(osic,
		                              "fcntl() require fd, cmd and optional value");
	}

	value = 0;
	if (argc == 3 && oobject_is_integer(osic, argv[2])) {
		value = ointeger_to_long(osic, argv[2]);
	}

	fd = ointeger_to_long(osic, argv[0]);
	cmd = ointeger_to_long(osic, argv[1]);
	flags = fcntl(fd, cmd, value);
	if (flags == -1) {
		perror("fcntl");
		return ointeger_create_from_long(osic, -1);
	}

	return ointeger_create_from_long(osic, flags);
}
#endif

static struct oobject *
os_select(struct osic *osic, struct oobject *self, int argc, struct oobject *argv[])
{
	int i;
	int n;
	int fd;
	int count;
	int maxfd;

	fd_set rfds;
	fd_set wfds;
	fd_set xfds;
	struct timeval tv;
	struct timeval *timeout;

	int rlen;
	int wlen;
	int xlen;
	struct oobject *rlist[256];
	struct oobject *wlist[256];
	struct oobject *xlist[256];

	struct oobject *item;
	struct oobject *items[3];

	if (argc != 3 && argc != 4) {
		return oobject_error_argument(osic, "select() required 3 arrays");
	}

	if (!oobject_is_array(osic, argv[0]) ||
	    !oobject_is_array(osic, argv[1]) ||
	    !oobject_is_array(osic, argv[2]))
	{
		return oobject_error_argument(osic, "select() required 3 arrays");
	}

	if (argc == 4 && !oobject_is_integer(osic, argv[3])) {
		return oobject_error_argument(osic, "select() 4th not integer");
	}

	FD_ZERO(&rfds);
	FD_ZERO(&wfds);
	FD_ZERO(&xfds);

	maxfd = 0;
	count = oarray_length(osic, argv[0]);
	for (i = 0; i < count; i++) {
		item = oarray_get_item(osic, argv[0], i);
		fd = ointeger_to_long(osic, item);
		if (maxfd < fd) {
			maxfd = fd;
		}
		FD_SET(fd, &rfds);
	}

	count = oarray_length(osic, argv[1]);
	for (i = 0; i < count; i++) {
		item = oarray_get_item(osic, argv[1], i);
		fd = ointeger_to_long(osic, item);
		if (maxfd < fd) {
			maxfd = fd;
		}
		FD_SET(fd, &wfds);
	}

	count = oarray_length(osic, argv[2]);
	for (i = 0; i < count; i++) {
		item = oarray_get_item(osic, argv[2], i);
		fd = ointeger_to_long(osic, item);
		if (maxfd < fd) {
			maxfd = fd;
		}
		FD_SET(fd, &xfds);
	}

	timeout = NULL;
	if (argc == 4) {
		long us = ointeger_to_long(osic, argv[3]);

		tv.tv_sec = us / 1000000;
		tv.tv_usec = us % 1000000;
		timeout = &tv;
	}

	n = select(maxfd + 1, &rfds, &wfds, &xfds, timeout);
	if (n == 0) {
		/* timeout */
		items[0] = oarray_create(osic, 0, NULL);
		items[1] = oarray_create(osic, 0, NULL);
		items[2] = oarray_create(osic, 0, NULL);

		return oarray_create(osic, 3, items);
	}

	if (n < 0 && errno != EINTR) {
		/* error */
		return oobject_error_type(osic, "select() %s", strerror(errno));
	}

	rlen = 0;
	wlen = 0;
	xlen = 0;
	items[0] = oarray_create(osic, rlen, rlist);
	items[1] = oarray_create(osic, wlen, wlist);
	items[2] = oarray_create(osic, xlen, xlist);
	if (n > 0) {
		for (i = 0; i < maxfd + 1; i++) {
			if (FD_ISSET(i, &rfds)) {
				item = ointeger_create_from_long(osic, i);
				rlist[rlen++] = item;
				if (rlen == 256) {
					oarray_append(osic, items[0], rlen, rlist);
					rlen = 0;
				}
			}

			if (FD_ISSET(i, &wfds)) {
				item = ointeger_create_from_long(osic, i);
				wlist[wlen++] = item;
				if (wlen == 256) {
					oarray_append(osic, items[1], wlen, wlist);
					wlen = 0;
				}
			}

			if (FD_ISSET(i, &xfds)) {
				item = ointeger_create_from_long(osic, i);
				xlist[xlen++] = item;
				if (xlen == 256) {
					oarray_append(osic, items[2], xlen, xlist);
					xlen = 0;
				}
			}
		}
	}
	oarray_append(osic, items[0], rlen, rlist);
	oarray_append(osic, items[1], wlen, wlist);
	oarray_append(osic, items[2], xlen, xlist);

	return oarray_create(osic, 3, items);
}

static struct oobject *
os_realpath(struct osic *osic, struct oobject *self, int argc, struct oobject *argv[])
{
	const char *path;
	char buffer[PATH_MAX];

#ifdef WINDOWS
	path = ostring_to_cstr(osic, argv[0]);
	if (GetFullPathName(path, PATH_MAX, buffer, NULL)) {
		return ostring_create(osic, buffer, strlen(buffer));
	}
#else
	path = realpath(ostring_to_cstr(osic, argv[0]), buffer);
	if (path) {
		return ostring_create(osic, path, strlen(path));
	}
#endif

	return osic->l_nil;
}

static struct oobject *
os_time(struct osic *osic, struct oobject *self, int argc, struct oobject *argv[])
{
	time_t t;

	time(&t);

	return ointeger_create_from_long(osic, t);
}

static struct oobject *
os_ctime(struct osic *osic, struct oobject *self, int argc, struct oobject *argv[])
{
	time_t t;
	char *ptr;
	char buf[26];

	if (argc != 1 || oobject_is_integer(osic, argv[0])) {
		return oobject_error_argument(osic, "required 1 integer argument");
	}
	t = ointeger_to_long(osic, argv[0]);

#ifdef WINDOWS
	(void)buf;
	ptr = ctime(&t);
#else
	ptr = ctime_r(&t, buf);
#endif

	return ostring_create(osic, ptr, strlen(ptr));
}

struct ltmobject {
	struct oobject object;

	struct tm tm;
};

static struct oobject *
ltmobject_method(struct osic *osic,
                 struct oobject *self,
                 int method, int argc, struct oobject *argv[])
{
	return oobject_default(osic, self, method, argc, argv);
}

static struct oobject *
os_gmtime(struct osic *osic, struct oobject *self, int argc, struct oobject *argv[])
{
	time_t t;
	struct tm *tm;
	struct ltmobject *tmobject;

	tmobject = oobject_create(osic, sizeof(struct ltmobject), ltmobject_method);

	if (argc != 1 || !oobject_is_integer(osic, argv[0])) {
		return oobject_error_argument(osic, "required 1 integer argument");
	}

	t = ointeger_to_long(osic, argv[0]);

#ifdef WINDOWS
	tm = gmtime(&t);
	if (!tm) {
		return NULL;
	}
	memcpy(&tmobject->tm, tm, sizeof(*tm));
#else
	tm = gmtime_r(&t, &tmobject->tm);
	if (!tm) {
		return NULL;
	}
#endif

	return (struct oobject *)tmobject;
}

static struct oobject *
os_strftime(struct osic *osic, struct oobject *self, int argc, struct oobject *argv[])
{
	char buf[256];
	const char *fmt;
	struct ltmobject *tm;

	if (argc != 2 || !oobject_is_string(osic, argv[0]) || argv[1]->l_method != ltmobject_method) {
		return oobject_error_argument(osic, "required 2 integer arguments");
	}

	tm = (struct ltmobject *)argv[1];
	fmt = ostring_to_cstr(osic, argv[0]);

	strftime(buf, sizeof(buf), fmt, &tm->tm);

	return ostring_create(osic, buf, strlen(buf));
}

static struct oobject *
os_exit(struct osic *osic, struct oobject *self, int argc, struct oobject *argv[])
{
	int status;

	status = 0;
	if (argc == 1) {
		if (!oobject_is_integer(osic, argv[0])) {
			return oobject_error_argument(osic, "required 1 integer argument");
		}

		status = ointeger_to_long(osic, argv[0]);
	}

	exit(status);

	return osic->l_nil;
}

struct oobject *
os_module(struct osic *osic)
{
	char *cstr;
	struct oobject *name;
	struct oobject *module;

#define SET_FUNCTION(value) do {                                             \
	cstr = #value ;                                                      \
	name = ostring_create(osic, cstr, strlen(cstr));                    \
	oobject_set_attr(osic,                                              \
	                 module,                                             \
	                 name,                                               \
	                 ofunction_create(osic, name, NULL, os_ ## value)); \
} while(0)

#define SET_INTEGER(value) do {                                    \
	cstr = #value ;                                            \
	name = ostring_create(osic, cstr, strlen(cstr));          \
	oobject_set_attr(osic,                                    \
	                 module,                                   \
	                 name,                                     \
	                 ointeger_create_from_long(osic, value)); \
} while(0)

	module = omodule_create(osic, ostring_create(osic, "os", 2));

	SET_FUNCTION(open);
	SET_FUNCTION(read);
	SET_FUNCTION(write);
	SET_FUNCTION(close);

	SET_FUNCTION(time);
	SET_FUNCTION(ctime);
	SET_FUNCTION(gmtime);
	SET_FUNCTION(strftime);
	SET_FUNCTION(select);
	SET_FUNCTION(realpath);
	SET_FUNCTION(exit);

#ifndef WINDOWS
	SET_FUNCTION(fcntl);
#endif

	/* common */
	SET_INTEGER(EXIT_SUCCESS);
	SET_INTEGER(EXIT_FAILURE);

	SET_INTEGER(O_RDONLY);
	SET_INTEGER(O_WRONLY);
	SET_INTEGER(O_RDWR);
	SET_INTEGER(O_APPEND);
	SET_INTEGER(O_CREAT);
	SET_INTEGER(O_TRUNC);
	SET_INTEGER(O_EXCL);
#ifdef WINDOWS
	SET_INTEGER(O_TEXT);
	SET_INTEGER(O_BINARY);

	SET_INTEGER(S_IREAD);
	SET_INTEGER(S_IWRITE);
#else
	SET_INTEGER(O_NONBLOCK);
	SET_INTEGER(O_NOFOLLOW);
	SET_INTEGER(O_CLOEXEC);

	SET_INTEGER(S_IRWXU);
	SET_INTEGER(S_IRUSR);
	SET_INTEGER(S_IWUSR);
	SET_INTEGER(S_IXUSR);
	SET_INTEGER(S_IRWXG);
	SET_INTEGER(S_IRGRP);
	SET_INTEGER(S_IWGRP);
	SET_INTEGER(S_IXGRP);
	SET_INTEGER(S_IRWXO);
	SET_INTEGER(S_IROTH);
	SET_INTEGER(S_IWOTH);
	SET_INTEGER(S_IXOTH);

	SET_INTEGER(F_GETFL);
	SET_INTEGER(O_NONBLOCK);
#endif

	return module;
}
