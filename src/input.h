#ifndef OSIC_INPUT_H
#define OSIC_INPUT_H

struct osic;

struct input {
	long size;   /* size of buffer */
	long offset; /* offset in buffer */
	long line;   /* line number */
	long column; /* position of current line */
	char *filename;

	char *buffer;
};

struct input *
input_create(struct osic *osic);

void
input_destroy(struct osic *osic,
              struct input *input);

int
input_set_file(struct osic *osic,
               const char *filename);

int
input_set_buffer(struct osic *osic,
                 const char *filename,
                 char *buffer,
                 int length);

char *
input_filename(struct osic *osic);

long
input_line(struct osic *osic);

long
input_column(struct osic *osic);

int
input_getchar(struct osic *osic);

void
input_ungetchar(struct osic *osic, int c);

#endif /* osic_INPUT_H */
