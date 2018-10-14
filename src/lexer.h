#ifndef OSIC_LEXER_H
#define OSIC_LEXER_H

struct osic;

struct lexer {
	int lookahead;

	long length;
	char *buffer; /* name, number and string's text value */
};

struct lexer *
lexer_create(struct osic *osic);

void
lexer_destroy(struct osic *osic, struct lexer *lexer);

long
lexer_get_length(struct osic *osic);

char *
lexer_get_buffer(struct osic *osic);

int
lexer_get_token(struct osic *osic);

int
lexer_next_token(struct osic *osic);

#endif /* osic_LEXER_H */
