#include "osic.h"
#include "hash.h"
#include "oArray.h"
#include "oString.h"
#include "oInteger.h"
#include "oIterator.h"
#include "lib/builtin.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static struct oobject *
ostring_format_string_function(struct osic *osic,
                               struct oobject *self,
                               int argc, struct oobject *argv[])
{
	struct oobject *callable;

	callable = oobject_get_attr(osic,
	                            argv[0],
	                            osic->l_string_string);
	if (callable && !oobject_is_error(osic, callable)) {
		return oobject_call(osic, callable, 0, NULL);
	}

	return oobject_string(osic, argv[0]);
}

static struct oobject *
ostring_format_callback(struct osic *osic,
                        struct oframe *frame,
                        struct oobject *retval)
{
	int i;
	int j;
	int k;
	int r;
	long n;
	long len;
	long count;
	struct oobject *item;
	struct oobject *array;
	struct ostring *string;
	struct ostring *newstring;

	array = retval;
	string = (struct ostring *)frame->self;

	count = 0;
	for (i = 0; i < string->length - 1; i++) {
		if (string->buffer[i] == '{' &&
		    string->buffer[i + 1] == '}')
		{
			count += 1;
		}
	}

	n = oarray_length(osic, array);
	if (count > n) {
		return oobject_error_item(osic,
		                          "'%@' index out of range",
		                          string);
	}

	len = string->length;
	for (i = 0; i < n; i++) {
		item = oarray_get_item(osic, array, i);
		len = len - 2 + ostring_length(osic, item);
	}

	j = 0;
	k = 0;
	newstring = ostring_create(osic, NULL, len);
	if (!newstring) {
		return NULL;
	}
	for (i = 0; i < string->length - 1; i++) {
		if (string->buffer[i] == '{' &&
		    string->buffer[i + 1] == '}')
		{
			item = oarray_get_item(osic, array, k++);
			memcpy(newstring->buffer + j,
			       ((struct ostring *)item)->buffer,
			       ((struct ostring *)item)->length);
			j += ((struct ostring *)item)->length;
			i += 1;
		} else {
			r = string->buffer[i];
			newstring->buffer[j++] = (char)r;
		}
	}
	newstring->buffer[j++] = string->buffer[i];

	return (struct oobject *)newstring;
}

static struct oobject *
ostring_format(struct osic *osic,
               struct oobject *self,
               int argc, struct oobject *argv[])
{
	struct oframe *frame;
	struct oobject *name;
	struct oobject *items[2];

	frame = osic_machine_push_new_frame(osic,
	                                     self,
	                                     NULL,
	                                     ostring_format_callback,
	                                     0);
	if (!frame) {
		return NULL;
	}

	name = ostring_create(osic, "ostring_format_string", 21);
	if (!name) {
		return NULL;
	}
	items[0] = ofunction_create(osic,
	                            name,
	                            self,
	                            ostring_format_string_function);
	if (!items[0]) {
		return NULL;
	}
	items[1] = oarray_create(osic, argc, argv);
	if (!items[1]) {
		return NULL;
	}

	return builtin_map(osic, NULL, 2, items);
}

static struct oobject *
ostring_join_array(struct osic *osic,
                   struct oobject *join,
                   struct oobject *array)
{
	long i;
	long n;
	long off;
	long len;
	struct oobject *item;
	struct ostring *string;

	n = oarray_length(osic, array);
	len = 0;
	for (i = 0; i < n; i++) {
		item = oarray_get_item(osic, array, i);
		if (!oobject_is_string(osic, item)) {
			const char *fmt;

			fmt = "'%@' required string value";
			return oobject_error_argument(osic, fmt, join);

		}

		len += ((struct ostring *)item)->length;
	}
	len += ((struct ostring *)join)->length * (n - 1);
	string = ostring_create(osic, NULL, len);
	if (!string) {
		return NULL;
	}

	off = 0;
	for (i = 0; i < n; i++) {
		item = oarray_get_item(osic, array, i);

		if (off) {
			memcpy(string->buffer + off,
			       ((struct ostring *)join)->buffer,
			       ((struct ostring *)join)->length);
			off += ((struct ostring *)join)->length;

			memcpy(string->buffer + off,
			       ((struct ostring *)item)->buffer,
			       ((struct ostring *)item)->length);
			off += ((struct ostring *)item)->length;
		} else {
			memcpy(string->buffer + off,
			       ((struct ostring *)item)->buffer,
			       ((struct ostring *)item)->length);
			off += ((struct ostring *)item)->length;
		}
	}

	if (string) {
		return (struct oobject *)string;
	}

	return osic->l_empty_string;
}

static struct oobject *
ostring_join_callback(struct osic *osic,
                      struct oframe *frame,
                      struct oobject *retval)
{
	return ostring_join_array(osic, frame->self, retval);
}

static struct oobject *
ostring_join_iterable(struct osic *osic,
                      struct oobject *join,
                      struct oobject *iterable)
{
	struct oframe *frame;

	frame = osic_machine_push_new_frame(osic,
	                                     join,
	                                     NULL,
	                                     ostring_join_callback,
	                                     0);
	if (!frame) {
		return NULL;
	}

	return oiterator_to_array(osic, iterable, 0);
}

static struct oobject *
ostring_join(struct osic *osic,
             struct oobject *self,
             int argc, struct oobject *argv[])
{
	if (oobject_is_array(osic, argv[0])) {
		return ostring_join_array(osic, self, argv[0]);
	}

	return ostring_join_iterable(osic, self, argv[0]);
}

static struct oobject *
ostring_trim(struct osic *osic,
             struct oobject *self,
             int argc, struct oobject *argv[])
{
	int i;
	int j;
	int k;
	int c;
	long nchars;
	const char *chars;
	struct ostring *string;

	chars = " \t\r\n";
	nchars = 4;
	if (argc) {
		if (argc != 1 || !oobject_is_string(osic, argv[0])) {
			const char *fmt;

			fmt = "'%@' accept 1 string argument";
			return oobject_error_argument(osic, fmt, self);
		}

		chars = ostring_to_cstr(osic, argv[0]);
		nchars = ostring_length(osic, argv[0]);
	}

	string = (struct ostring *)self;

	for (i = 0; i < string->length; i++) {
		c = string->buffer[i];
		for (k = 0; k < nchars; k++) {
			if (c == chars[k]) {
				break;
			}
		}
		if (k == nchars) {
			break;
		}
	}

	for (j = (int)string->length; j > 0; j--) {
		c = string->buffer[j - 1];
		for (k = 0; k < nchars; k++) {
			if (c == chars[k]) {
				break;
			}
		}
		if (k == nchars) {
			break;
		}
	}

	if (j <= i) {
		return osic->l_empty_string;
	}

	return ostring_create(osic, &string->buffer[i], j - i);
}

static struct oobject *
ostring_ltrim(struct osic *osic,
              struct oobject *self,
              int argc, struct oobject *argv[])
{
	int i;
	int k;
	int c;
	long nchars;
	const char *chars;
	struct ostring *string;

	chars = " \t\r\n";
	nchars = 4;
	if (argc) {
		if (argc != 1 || !oobject_is_string(osic, argv[0])) {
			const char *fmt;

			fmt = "'%@' accept 1 string argument";
			return oobject_error_argument(osic, fmt, self);
		}

		chars = ostring_to_cstr(osic, argv[0]);
		nchars = ostring_length(osic, argv[0]);
	}

	string = (struct ostring *)self;

	for (i = 0; i < string->length; i++) {
		c = string->buffer[i];
		for (k = 0; k < nchars; k++) {
			if (c == chars[k]) {
				break;
			}
		}
		if (k == nchars) {
			break;
		}
	}

	if (i == string->length) {
		return osic->l_empty_string;
	}

	return ostring_create(osic, &string->buffer[i], string->length - i);
}

static struct oobject *
ostring_rtrim(struct osic *osic,
              struct oobject *self,
              int argc, struct oobject *argv[])
{
	int j;
	int k;
	int c;
	long nchars;
	const char *chars;
	struct ostring *string;

	chars = " \t\r\n";
	nchars = 4;
	if (argc) {
		if (argc != 1 || !oobject_is_string(osic, argv[0])) {
			const char *fmt;

			fmt = "'%@' accept 1 string argument";
			return oobject_error_argument(osic, fmt, self);
		}

		chars = ostring_to_cstr(osic, argv[0]);
		nchars = ostring_length(osic, argv[0]);
	}

	string = (struct ostring *)self;

	for (j = (int)string->length; j > 0; j--) {
		c = string->buffer[j - 1];
		for (k = 0; k < nchars; k++) {
			if (c == chars[k]) {
				break;
			}
		}
		if (k == nchars) {
			break;
		}
	}

	if (j == 0) {
		return osic->l_empty_string;
	}

	return ostring_create(osic, string->buffer, j);
}

static struct oobject *
ostring_lower(struct osic *osic,
              struct oobject *self,
              int argc, struct oobject *argv[])
{
	int i;
	struct ostring *string;
	struct ostring *newstring;

	string = (struct ostring *)self;
	newstring = ostring_create(osic, NULL, string->length);
	if (!newstring) {
		return NULL;
	}

	for (i = 0; i < string->length; i++) {
		newstring->buffer[i] = (char)tolower(string->buffer[i]);
	}

	return (struct oobject *)newstring;
}

static struct oobject *
ostring_upper(struct osic *osic,
              struct oobject *self,
              int argc, struct oobject *argv[])
{
	int i;
	struct ostring *string;
	struct ostring *newstring;

	string = (struct ostring *)self;
	newstring = ostring_create(osic, NULL, string->length);
	if (!newstring) {
		return NULL;
	}

	for (i = 0; i < string->length; i++) {
		newstring->buffer[i] = (char)toupper(string->buffer[i]);
	}

	return (struct oobject *)newstring;
}

static struct oobject *
ostring_find(struct osic *osic,
             struct oobject *self,
             int argc, struct oobject *argv[])
{
	int i;
	int c;
	long len;
	struct ostring *string;
	struct ostring *substring;

	if (argc && oobject_is_string(osic, argv[0])) {
		string = (struct ostring *)self;
		substring = (struct ostring *)argv[0];

		if (substring->length == 0) {
			return ointeger_create_from_long(osic, 0);
		}

		if (substring->length == 1) {
			c = substring->buffer[0];
			for (i = 0; i < string->length; i++) {
				if (string->buffer[i] == c) {
					return ointeger_create_from_long(osic,
					                                 i);
				}
			}

			return ointeger_create_from_long(osic, -1);
		}

		len = string->length - substring->length + 1;
		for (i = 0; i < len; i++) {
			if (strncmp(string->buffer + i,
			            substring->buffer,
			            substring->length) == 0)
			{
				return ointeger_create_from_long(osic, i);
			}
		}
	}

	return ointeger_create_from_long(osic, -1);
}

static struct oobject *
ostring_rfind(struct osic *osic,
              struct oobject *self,
              int argc, struct oobject *argv[])
{
	int c;
	long i;
	long len;
	struct ostring *string;
	struct ostring *substring;

	if (argc && oobject_is_string(osic, argv[0])) {
		string = (struct ostring *)self;
		substring = (struct ostring *)argv[0];

		if (substring->length == 0) {
			return ointeger_create_from_long(osic, 0);
		}

		if (substring->length == 1) {
			c = substring->buffer[0];
			for (i = string->length; i > 0; i--) {
				if (string->buffer[i - 1] == c) {
					return ointeger_create_from_long(osic,
					                                 i - 1);
				}
			}

			return ointeger_create_from_long(osic, -1);
		}

		len = string->length - substring->length + 1;
		for (i = len; i > 0; i--) {
			if (strncmp(string->buffer + i - 1,
			            substring->buffer,
			            substring->length) == 0)
			{
				return ointeger_create_from_long(osic, i - 1);
			}
		}
	}

	return ointeger_create_from_long(osic, -1);
}

static struct oobject *
ostring_replace(struct osic *osic,
                struct oobject *self,
                int argc, struct oobject *argv[])
{
	int i;
	int j;
	int c;
	int r;
	int o;
	long len;
	long diff;
	int count;
	struct ostring *string;
	struct ostring *substring;
	struct ostring *repstring;
	struct ostring *newstring;

	if (argc == 2 &&
	    oobject_is_string(osic, argv[0]) &&
	    oobject_is_string(osic, argv[1]))
	{
		string = (struct ostring *)self;
		substring = (struct ostring *)argv[0];
		repstring = (struct ostring *)argv[1];

		if (substring->length == 0) {
			return self;
		}

		count = 0;
		if (substring->length == 1) {
			c = substring->buffer[0];
			for (i = 0; i < string->length; i++) {
				if (string->buffer[i] == c) {
					count += 1;
				}
			}
		} else {
			len = string->length - substring->length + 1;
			for (i = 0; i < len; i++) {
				if (strncmp(string->buffer + i,
				            substring->buffer,
				            substring->length) == 0)
				{
					count += 1;
				}
			}
		}
		if (!count) {
			return self;
		}

		diff = repstring->length - substring->length;
		len = string->length + diff * count;
		newstring = ostring_create(osic, NULL, len);
		if (!newstring) {
			return NULL;
		}

		j = 0;
		/* replace('x', 'y') */
		if (substring->length == 1 && repstring->length == 1) {
			c = substring->buffer[0];
			r = repstring->buffer[0];
			memcpy(newstring->buffer,
			       string->buffer,
			       string->length);
			for (i = 0; i < newstring->length; i++) {
				if (newstring->buffer[i] == c) {
					newstring->buffer[j++] = (char)r;
				} else {
					o = string->buffer[i];
					newstring->buffer[j++] = (char)o;
				}
			}

		/* replace('x', 'yyy') */
		} else if (substring->length == 1) {
			c = substring->buffer[0];
			for (i = 0; i < string->length; i++) {
				if (string->buffer[i] == c) {
					if (repstring->length >= 1) {
						memcpy(newstring->buffer + j,
						       repstring->buffer,
						       repstring->length);
						j += repstring->length;
					}
				} else {
					o = string->buffer[i];
					newstring->buffer[j++] = (char)o;
				}
			}

		/* replace('xxx', 'y') */
		} else if (repstring->length == 1) {
			for (i = 0; i < string->length; i++) {
				r = repstring->buffer[0];
				if (string->length - i >= substring->length &&
				    strncmp(string->buffer + i,
				            substring->buffer,
				            substring->length) == 0)
				{
					newstring->buffer[j++] = (char)r;
					i += substring->length - 1;
				} else {
					o = string->buffer[i];
					newstring->buffer[j++] = (char)o;
				}
			}

		/* replace('xxx', 'yyy') */
		} else {
			for (i = 0; i < string->length; i++) {
				if (string->length - i >= substring->length &&
				    strncmp(string->buffer + i,
				            substring->buffer,
				            substring->length) == 0)
				{
					memcpy(newstring->buffer + j,
					       repstring->buffer,
					       repstring->length);
					j += repstring->length;
					i += substring->length - 1;
				} else {
					o = string->buffer[i];
					newstring->buffer[j++] = (char)o;
				}
			}
		}

		return (struct oobject *)newstring;
	}

	return osic->l_nil;
}

static struct oobject *
ostring_split(struct osic *osic,
              struct oobject *self,
              int argc, struct oobject *argv[])
{
	int c;
	long i;
	long j;
	long max;
	long len;
	struct ostring *string;
	struct ostring *substring;
	struct oobject *item;
	struct oobject *array;

	if (argc && oobject_is_string(osic, argv[0])) {
		string = (struct ostring *)self;
		substring = (struct ostring *)argv[0];

		if (substring->length == 0) {
			return oarray_create(osic, 1, &self);
		}

		if (argc == 2 && oobject_is_integer(osic, argv[1])) {
			max = ointeger_to_long(osic, argv[1]);
		} else {
			max = 0;
			if (substring->length == 1) {
				c = substring->buffer[0];
				for (i = 0; i < string->length; i++) {
					if (string->buffer[i] == c) {
						max += 1;
					}
				}
			} else {
				len = string->length - substring->length + 1;
				for (i = 0; i < len; i++) {
					if (strncmp(string->buffer + i,
					            substring->buffer,
					            substring->length) == 0)
					{
						max += 1;
					}
				}
			}
		}

		if (max == 0) {
			return oarray_create(osic, 1, &self);
		}

		array = oarray_create(osic, 0, NULL);
		if (!array) {
			return NULL;
		}
		j = 0;
		if (substring->length == 1) {
			c = substring->buffer[0];
			for (i = 0; i < string->length && max > 0; i++) {
				if (string->buffer[i] != c) {
					continue;
				}
				item = ostring_create(osic,
				                      string->buffer + j,
				                      i - j);
				if (!item) {
					return NULL;
				}
				if (!oarray_append(osic, array, 1, &item)) {
					return NULL;
				}

				j = i + 1;
				max -= 1;
			}

			if (j < string->length) {
				item = ostring_create(osic,
				                      string->buffer + j,
				                      string->length - j);
				if (!item) {
					return NULL;
				}
				if (!oarray_append(osic, array, 1, &item)) {
					return NULL;
				}
			}

			return array;
		}

		len = string->length - substring->length + 1;
		for (i = 0; i < len && max > 0; i++) {
			if (strncmp(string->buffer + i,
			            substring->buffer,
			            substring->length) != 0)
			{
				continue;
			}
			item = ostring_create(osic,
			                      string->buffer + j,
			                      i - j);
			if (!item) {
				return NULL;
			}
			if (!oarray_append(osic, array, 1, &item)) {
				return NULL;
			}
			j = i + substring->length;
			i = i + substring->length - 1;
			max -= 1;
		}
		if (j < string->length) {
			item = ostring_create(osic,
			                      string->buffer + j,
			                      string->length - j);
			if (!item) {
				return NULL;
			}
			if (!oarray_append(osic, array, 1, &item)) {
				return NULL;
			}
		}

		return array;
	}

	return osic->l_false;
}

static struct oobject *
ostring_startswith(struct osic *osic,
                   struct oobject *self,
                   int argc, struct oobject *argv[])
{
	struct ostring *string;
	struct ostring *substring;

	if (argc && oobject_is_string(osic, argv[0])) {
		string = (struct ostring *)self;
		substring = (struct ostring *)argv[0];

		if (substring->length == 0) {
			return osic->l_true;
		}
		if (substring->length > string->length) {
			return osic->l_false;
		}

		if (memcmp(string->buffer,
		           substring->buffer,
		           substring->length) == 0)
		{
			return osic->l_true;
		}
	}

	return osic->l_false;
}

static struct oobject *
ostring_endswith(struct osic *osic,
                 struct oobject *self,
                 int argc, struct oobject *argv[])
{
	struct ostring *string;
	struct ostring *substring;

	if (argc && oobject_is_string(osic, argv[0])) {
		string = (struct ostring *)self;
		substring = (struct ostring *)argv[0];

		if (substring->length == 0) {
			return osic->l_true;
		}
		if (substring->length > string->length) {
			return osic->l_false;
		}

		if (memcmp(string->buffer + string->length - substring->length,
		           substring->buffer,
		           substring->length) == 0)
		{
			return osic->l_true;
		}
	}

	return osic->l_false;
}

static struct oobject *
ostring_add(struct osic *osic, struct ostring *a, struct ostring *b)
{
	struct ostring *string;

	if (!oobject_is_string(osic, (struct oobject *)b)) {
		return oobject_error_type(osic,
		                          "'%@' unsupport operand '%@'",
		                          a,
		                          b);
	}

	string = ostring_create(osic, NULL, a->length + b->length);
	if (!string) {
		return NULL;
	}
	memcpy(string->buffer, a->buffer, a->length);
	memcpy(string->buffer + a->length, b->buffer, b->length);
	string->buffer[string->length] = '\0';

	return (struct oobject *)string;
}

static struct oobject *
ostring_get_item(struct osic *osic,
                 struct ostring *self, struct oobject *name)
{
	long i;
	char buffer[1];

	if (oobject_is_integer(osic, name)) {
		i = ointeger_to_long(osic, name);
		if (i < self->length) {
			if (i < 0) {
				i = self->length + i;
			}
			buffer[0] = self->buffer[i];

			return ostring_create(osic, buffer, 1);
		}

		return NULL;
	}

	return NULL;
}

static struct oobject *
ostring_has_item(struct osic *osic,
                 struct oobject *self,
                 struct oobject *items)
{
	struct oobject *location;

	location = ostring_find(osic, self, 1, &items);
	if (ointeger_to_long(osic, location) == -1) {
		return osic->l_false;
	}
	return osic->l_true;
}

static struct oobject *
ostring_get_attr(struct osic *osic,
                 struct oobject *self,
                 struct oobject *name)
{
	const char *cstr;

	cstr = ostring_to_cstr(osic, name);
	if (strcmp(cstr, "upper") == 0) {
		return ofunction_create(osic, name, self, ostring_upper);
	}

	if (strcmp(cstr, "lower") == 0) {
		return ofunction_create(osic, name, self, ostring_lower);
	}

	if (strcmp(cstr, "trim") == 0) {
		return ofunction_create(osic, name, self, ostring_trim);
	}

	if (strcmp(cstr, "ltrim") == 0) {
		return ofunction_create(osic, name, self, ostring_ltrim);
	}

	if (strcmp(cstr, "rtrim") == 0) {
		return ofunction_create(osic, name, self, ostring_rtrim);
	}

	if (strcmp(cstr, "find") == 0) {
		return ofunction_create(osic, name, self, ostring_find);
	}

	if (strcmp(cstr, "rfind") == 0) {
		return ofunction_create(osic, name, self, ostring_rfind);
	}

	if (strcmp(cstr, "replace") == 0) {
		return ofunction_create(osic, name, self, ostring_replace);
	}

	if (strcmp(cstr, "split") == 0) {
		return ofunction_create(osic, name, self, ostring_split);
	}

	if (strcmp(cstr, "join") == 0) {
		return ofunction_create(osic, name, self, ostring_join);
	}

	if (strcmp(cstr, "format") == 0) {
		return ofunction_create(osic, name, self, ostring_format);
	}

	if (strcmp(cstr, "startswith") == 0) {
		return ofunction_create(osic, name, self, ostring_startswith);
	}

	if (strcmp(cstr, "endswith") == 0) {
		return ofunction_create(osic, name, self, ostring_endswith);
	}

	return NULL;
}

static struct oobject *
ostring_get_slice(struct osic *osic,
                  struct ostring *self,
                  struct oobject *start,
                  struct oobject *stop,
                  struct oobject *step)
{
	long off;
	long istart;
	long istop;
	long istep;
	struct ostring *string;

	istart = ointeger_to_long(osic, start);
	if (stop == osic->l_nil) {
		istop = self->length;
	} else {
		istop = ointeger_to_long(osic, stop);
	}
	istep = ointeger_to_long(osic, step);

	if (istart < 0) {
		istart = self->length + istart;
	}
	if (istop < 0) {
		istop = self->length + istop;
	}
	if (istart >= istop) {
		return osic->l_empty_string;
	}

	string = ostring_create(osic, NULL, (istop - istart) / istep);
	if (string) {
		off = 0;
		for (; istart < istop; istart += istep) {
			string->buffer[off++] = self->buffer[istart];
		}
	}

	return (struct oobject *)string;
}

static struct oobject *
ostring_method(struct osic *osic,
               struct oobject *self,
               int method, int argc, struct oobject *argv[])
{
#define cast(a) ((struct ostring *)(a))

#define cmpop(op) do {                                            \
	if (oobject_is_string(osic, argv[0])) {                  \
		if (strcmp(ostring_to_cstr(osic, self),          \
		           ostring_to_cstr(osic, argv[0])) op 0) \
		{                                                 \
			return osic->l_true;                     \
		}                                                 \
		return osic->l_false;                            \
	}                                                         \
	return oobject_default(osic, self, method, argc, argv);  \
} while (0)

	switch (method) {
	case OOBJECT_METHOD_LT:
		cmpop(<);

	case OOBJECT_METHOD_LE:
		cmpop(<=);

	case OOBJECT_METHOD_EQ:
		cmpop(==);

	case OOBJECT_METHOD_NE:
		cmpop(!=);

	case OOBJECT_METHOD_GE:
		cmpop(>=);

	case OOBJECT_METHOD_GT:
		cmpop(>);

	case OOBJECT_METHOD_ADD:
		return ostring_add(osic, cast(self), cast(argv[0]));

	case OOBJECT_METHOD_GET_ITEM:
		return ostring_get_item(osic, cast(self), argv[0]);

	case OOBJECT_METHOD_HAS_ITEM:
		return ostring_has_item(osic, self, argv[0]);

	case OOBJECT_METHOD_GET_ATTR:
		return ostring_get_attr(osic, self, argv[0]);

	case OOBJECT_METHOD_GET_SLICE:
		return ostring_get_slice(osic,
		                         cast(self),
		                         argv[0],
		                         argv[1],
		                         argv[2]);

	case OOBJECT_METHOD_HASH:
		return ointeger_create_from_long(osic,
		                                 osic_hash(osic,
		                                            cast(self)->buffer,
		                                            cast(self)->length));

	case OOBJECT_METHOD_STRING:
		return self;

	case OOBJECT_METHOD_LENGTH:
		return ointeger_create_from_long(osic, cast(self)->length);

	case OOBJECT_METHOD_BOOLEAN:
		if (cast(self)->length) {
			return osic->l_true;
		}
		return osic->l_false;

	case OOBJECT_METHOD_DESTROY:
		return NULL;

	default:
		return oobject_default(osic, self, method, argc, argv);
	}
}

const char *
ostring_to_cstr(struct osic *osic, struct oobject *object)
{
	return ((struct ostring *)object)->buffer;
}

char *
ostring_buffer(struct osic *osic, struct oobject *object)
{
	return ((struct ostring *)object)->buffer;
}

long
ostring_length(struct osic *osic, struct oobject *object)
{
	return ((struct ostring *)object)->length;
}

void *
ostring_create(struct osic *osic, const char *buffer, long length)
{
	struct ostring *self;

	/*
	 * ostring->buffer has one byte more then length for '\0'
	 */
	self = oobject_create(osic, sizeof(*self) + length, ostring_method);
	if (self) {
		self->length = length;

		if (buffer) {
			memcpy(self->buffer, buffer, length);
			self->buffer[length] = '\0';
		}
	}

	return self;
}

static struct oobject *
ostring_type_method(struct osic *osic,
                    struct oobject *self,
                    int method, int argc, struct oobject *argv[])
{
	switch (method) {
	case OOBJECT_METHOD_CALL:
		if (argc) {
			return oobject_string(osic, argv[0]);
		}
		return osic->l_empty_string;

	case OOBJECT_METHOD_CALLABLE:
		return osic->l_true;

	default:
		return oobject_default(osic, self, method, argc, argv);
	}
}

struct otype *
ostring_type_create(struct osic *osic)
{
	struct otype *type;

	type = otype_create(osic,
	                    "string",
	                    ostring_method,
	                    ostring_type_method);
	if (type) {
		osic_add_global(osic, "string", type);
	}

	return type;
}
