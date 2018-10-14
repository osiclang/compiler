#include "osic.h"
#include "token.h"
#include "input.h"
#include "lexer.h"
#include "symbol.h"

#include <ctype.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

struct lexer *
lexer_create(struct osic *osic)
{
	struct lexer *lexer;

	lexer = osic_allocator_alloc(osic, sizeof(*lexer));
	if (!lexer) {
		return NULL;
	}
	memset(lexer, 0, sizeof(*lexer));

	return lexer;
}

void
lexer_destroy(struct osic *osic, struct lexer *lexer)
{
	osic_allocator_free(osic, lexer);
}

long
lexer_get_length(struct osic *osic)
{
	struct lexer *lexer;

	lexer = osic->l_lexer;
	return lexer->length;
}

char *
lexer_get_buffer(struct osic *osic)
{
	struct lexer *lexer;

	lexer = osic->l_lexer;
	return lexer->buffer;
}

int
lexer_get_token(struct osic *osic)
{
	struct lexer *lexer;

	lexer = osic->l_lexer;
	return lexer->lookahead;
}

void
lexer_scan_number(struct osic *osic, int c)
{
	int dot;
	int oct;
	int hex;
	int done;
	long offset;
	long length;
	char *buffer;
	struct lexer *lexer;

	assert(isdigit(c) || c == '.');

	dot = 0;
	oct = 0;
	hex = 0;
	done = 0;
	offset = 0;
	length = OSIC_NAME_MAX;
	lexer = osic->l_lexer;
	buffer = osic_allocator_alloc(osic, length);
	if (c == '0') {
		oct = 1;
	}
	while (!done) {
		switch (c) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
			break;
		case '8':
		case '9':
			if (oct) {
				done = 1;
			}
			break;
		case 'a':
		case 'A':
		case 'b':
		case 'B':
		case 'c':
		case 'C':
		case 'd':
		case 'D':
		case 'e':
		case 'E':
		case 'f':
		case 'F':
			if (!hex) {
				done = 1;
			}
			break;
		case 'x':
		case 'X':
			if (oct && offset == 1) {
				oct = 0;
				hex = 1;
			} else {
				done = 1;
			}
			break;
		case '.':
			if (hex || dot) {
				done = 1;
			} else {
				oct = 0;
				dot = 1;
			}
			break;
		default:
			done = 1;
		}

		if (done) {
			break;
		}

		buffer[offset++] = (char)c;
		c = input_getchar(osic);
		if (offset == length) {
			length += OSIC_NAME_MAX;
			buffer = osic_allocator_realloc(osic, buffer, length);
			if (!buffer) {
				lexer->lookahead = TOKEN_ERROR;
				return;
			}
		}
	}

	/* get out of last '.' */
	if (offset && buffer[offset - 1] == '.') {
		offset -= 1;
		if (c != '.') {
			input_ungetchar(osic, c);
		}
	}
	buffer[offset] = '\0';
	input_ungetchar(osic, c);
	lexer->lookahead = TOKEN_NUMBER;
	lexer->length = offset;
	lexer->buffer = buffer;
}

void
lexer_scan_string(struct osic *osic, int c)
{
	int single;
	long offset;
	long length;
	char *buffer;
	struct lexer *lexer;

	lexer = osic->l_lexer;

	single = 0;
	if (c == '\'') {
		single = 1;
	}

	offset = 0;
	length = OSIC_NAME_MAX;
	buffer = osic_allocator_alloc(osic, length);
	c = input_getchar(osic);
	for (;;) {
		if (c == '\0') {
			fprintf(stderr,
			        "%s:%ld:%ld: error: <eof> in string literal\n",
			        input_filename(osic),
			        input_line(osic) + 1,
			        input_column(osic) + 1);
			lexer->lookahead = TOKEN_ERROR;
			osic_allocator_free(osic, buffer);
			return;
		}

		if ((single && c == '\'') || (!single && c == '"')) {
			break;
		}

		if (c == '\\') {
			c = input_getchar(osic);
			switch (c) {
			case 'n':
				c = '\n';
				break;

			case 't':
				c = '\t';
				break;

			case 'r':
				c = '\r';
				break;

			case '\'':
				c = '\'';
				break;

			case '"':
				c = '"';
				break;

			case '\\':
				c = '\\';
				break;

			default:
				break;
			}
		}
		buffer[offset++] = (char)c;
		if (offset == length) {
			length += OSIC_NAME_MAX;
			buffer = osic_allocator_realloc(osic, buffer, length);
		}
		c = input_getchar(osic);
	}
	buffer[offset] = '\0';
	/* because last "'" or '"' we don't need ungetchar() */
	lexer->lookahead = TOKEN_STRING;
	lexer->length = offset;
	lexer->buffer = buffer;
}

void
lexer_scan_name(struct osic *osic, int c)
{
	long offset;
	long length;
	char *buffer;
	struct lexer *lexer;

	assert(c == '_' || isalpha(c));

	offset = 0;
	length = OSIC_NAME_MAX;
	lexer = osic->l_lexer;
	buffer = osic_allocator_alloc(osic, length);
	while (c == '_' || isalnum(c)) {
		buffer[offset++] = (char)c;
		if (offset == OSIC_NAME_MAX - 1) {
			buffer[offset] = '\0';

			fprintf(stderr,
			        "%s:%ld:%ld: error: "
			        "name '%s...' too long,"
			        " 255 char max\n",
			        input_filename(osic),
			        input_line(osic) + 1,
			        input_column(osic) + 1,
			        lexer->buffer);
			lexer->lookahead = TOKEN_ERROR;
			osic_allocator_free(osic, buffer);

			return;
		}
		c = input_getchar(osic);
	}
	buffer[offset] = '\0';
	input_ungetchar(osic, c);
	lexer->lookahead = TOKEN_NAME;
	lexer->length = offset;
	lexer->buffer = buffer;
}

void
lexer_scan_line_comment(struct osic *osic, int c)
{
	while (c != '\n') {
		c = input_getchar(osic);
	}
}

void
lexer_scan_block_comment(struct osic *osic, int c)
{
	while (c != '\0') {
		if (c == '*') {
			c = input_getchar(osic);
			if (c == '/') {
				return;
			}
		} else {
			c = input_getchar(osic);
		}
	}

	input_ungetchar(osic, c);
}

int
lexer_is_keyword(struct osic *osic, char *keyword)
{
	struct lexer *lexer;

	lexer = osic->l_lexer;
	return (strncmp(lexer->buffer, keyword, OSIC_NAME_MAX) == 0);
}

void
lexer_scan_keyword(struct osic *osic, int c)
{
	struct lexer *lexer;

	lexer_scan_name(osic, c);

	lexer = osic->l_lexer;
	if (lexer->lookahead != TOKEN_NAME) {
		return;
	}

	if (lexer_is_keyword(osic, "if")) {
		lexer->lookahead = TOKEN_IF;
	} else if (lexer_is_keyword(osic, "else")) {
		lexer->lookahead = TOKEN_ELSE;
	} else if (lexer_is_keyword(osic, "for")) {
		lexer->lookahead = TOKEN_FOR;
	} else if (lexer_is_keyword(osic, "in")) {
		lexer->lookahead = TOKEN_IN;
	} else if (lexer_is_keyword(osic, "while")) {
		lexer->lookahead = TOKEN_WHILE;
	} else if (lexer_is_keyword(osic, "break")) {
		lexer->lookahead = TOKEN_BREAK;
	} else if (lexer_is_keyword(osic, "continue")) {
		lexer->lookahead = TOKEN_CONTINUE;
	} else if (lexer_is_keyword(osic, "delete")) {
		lexer->lookahead = TOKEN_DELETE;
	} else if (lexer_is_keyword(osic, "return")) {
		lexer->lookahead = TOKEN_RETURN;
	} else if (lexer_is_keyword(osic, "import")) {
		lexer->lookahead = TOKEN_IMPORT;
	} else if (lexer_is_keyword(osic, "as")) {
		lexer->lookahead = TOKEN_AS;
	} else if (lexer_is_keyword(osic, "def")) {
		lexer->lookahead = TOKEN_DEFINE;
	} else if (lexer_is_keyword(osic, "define")) {
		lexer->lookahead = TOKEN_DEFINE;
	} else if (lexer_is_keyword(osic, "function")) {
		lexer->lookahead = TOKEN_DEFINE;
	} else if (lexer_is_keyword(osic, "var")) {
		lexer->lookahead = TOKEN_VAR;
	} else if (lexer_is_keyword(osic, "try")) {
		lexer->lookahead = TOKEN_TRY;
	} else if (lexer_is_keyword(osic, "catch")) {
		lexer->lookahead = TOKEN_CATCH;
	} else if (lexer_is_keyword(osic, "finally")) {
		lexer->lookahead = TOKEN_FINALLY;
	} else if (lexer_is_keyword(osic, "throw")) {
		lexer->lookahead = TOKEN_THROW;
	} else if (lexer_is_keyword(osic, "nil")) {
		lexer->lookahead = TOKEN_NIL;
	} else if (lexer_is_keyword(osic, "true")) {
		lexer->lookahead = TOKEN_TRUE;
	} else if (lexer_is_keyword(osic, "false")) {
		lexer->lookahead = TOKEN_FALSE;
	} else if (lexer_is_keyword(osic, "sentinel")) {
		lexer->lookahead = TOKEN_SENTINEL;
	} else if (lexer_is_keyword(osic, "class")) {
		lexer->lookahead = TOKEN_CLASS;
	} else if (lexer_is_keyword(osic, "self")) {
		lexer->lookahead = TOKEN_SELF;
	} else if (lexer_is_keyword(osic, "this")) {
		lexer->lookahead = TOKEN_SELF;
	} else if (lexer_is_keyword(osic, "super")) {
		lexer->lookahead = TOKEN_SUPER;
	} else if (lexer_is_keyword(osic, "getter")) {
		lexer->lookahead = TOKEN_ACCESSOR_GETTER;
	} else if (lexer_is_keyword(osic, "setter")) {
		lexer->lookahead = TOKEN_ACCESSOR_SETTER;
	} else {
		lexer->lookahead = TOKEN_NAME;
	}
}

int
lexer_next_token(struct osic *osic)
{
	int c;
	struct lexer *lexer;

	c = input_getchar(osic);
	while (isspace(c)) {
		c = input_getchar(osic);
	}
	lexer = osic->l_lexer;
	osic_allocator_free(osic, lexer->buffer);
	lexer->length = 0;
	lexer->buffer = NULL;

	if (c == '_' || isalpha(c)) {
		lexer_scan_keyword(osic, c);
	} else if (isdigit(c)) {
		lexer_scan_number(osic, c);
	} else {
		switch (c) {
		case '\0':
			lexer->lookahead = TOKEN_EOF;
			break;

		case '#':
			lexer_scan_line_comment(osic, c);
			return lexer_next_token(osic);

		case '\'':
			/* FALLTHROUGH */
		case '"':
			lexer_scan_string(osic, c);
			break;

		case '.':
			c = input_getchar(osic);
			if (isdigit(c)) {
				input_ungetchar(osic, c);
				lexer_scan_number(osic, '.');
			} else {
				input_ungetchar(osic, c);
				lexer->lookahead = TOKEN_DOT;
			}
			break;

		case ',':
			lexer->lookahead = TOKEN_COMMA;
			break;

		case ';':
			lexer->lookahead = TOKEN_SEMICON;
			break;

		case '(':
			lexer->lookahead = TOKEN_LPAREN;
			break;

		case ')':
			lexer->lookahead = TOKEN_RPAREN;
			break;

		case '[':
			lexer->lookahead = TOKEN_LBRACK;
			break;

		case ']':
			lexer->lookahead = TOKEN_RBRACK;
			break;

		case '{':
			lexer->lookahead = TOKEN_LBRACE;
			break;

		case '}':
			lexer->lookahead = TOKEN_RBRACE;
			break;

		case '+':
			c = input_getchar(osic);
			if (c == '=') {
				lexer->lookahead = TOKEN_ADD_ASSIGN;
			} else {
				input_ungetchar(osic, c);
				lexer->lookahead = TOKEN_ADD;
			}
			break;

		case '-':
			c = input_getchar(osic);
			if (c == '=') {
				lexer->lookahead = TOKEN_SUB_ASSIGN;
			} else {
				input_ungetchar(osic, c);
				lexer->lookahead = TOKEN_SUB;
			}
			break;

		case '*':
			c = input_getchar(osic);
			if (c == '=') {
				lexer->lookahead = TOKEN_MUL_ASSIGN;
			} else {
				input_ungetchar(osic, c);
				lexer->lookahead = TOKEN_MUL;
			}
			break;

		case '/':
			c = input_getchar(osic);
			if (c == '=') {
				lexer->lookahead = TOKEN_DIV_ASSIGN;
			} else if (c == '/') {
				lexer_scan_line_comment(osic, c);
				return lexer_next_token(osic);
			} else if (c == '*') {
				lexer_scan_block_comment(osic, c);
				return lexer_next_token(osic);
			} else {
				input_ungetchar(osic, c);
				lexer->lookahead = TOKEN_DIV;
			}
			break;

		case '%':
			c = input_getchar(osic);
			if (c == '=') {
				lexer->lookahead = TOKEN_MOD_ASSIGN;
			} else {
				input_ungetchar(osic, c);
				lexer->lookahead = TOKEN_MOD;
			}
			break;

		case '=':
			c = input_getchar(osic);
			if (c == '=') {
				lexer->lookahead = TOKEN_EQ;
			} else {
				input_ungetchar(osic, c);
				lexer->lookahead = TOKEN_ASSIGN;
			}
			break;

		case '@':
			lexer->lookahead = TOKEN_ACCESSOR;
			break;

		case '!':
			c = input_getchar(osic);
			if (c == '=') {
				lexer->lookahead = TOKEN_NE;
			} else {
				input_ungetchar(osic, c);
				lexer->lookahead = TOKEN_LOGICAL_NOT;
			}
			break;

		case '>':
			c = input_getchar(osic);
			if (c == '=') {
				lexer->lookahead = TOKEN_GE;
			} else if (c == '>') {
				c = input_getchar(osic);
				if (c == '=') {
					lexer->lookahead = TOKEN_SHR_ASSIGN;
				} else {
					input_ungetchar(osic, c);
					lexer->lookahead = TOKEN_SHR;
				}
			} else {
				input_ungetchar(osic, c);
				lexer->lookahead = TOKEN_GT;
			}
			break;

		case '<':
			c = input_getchar(osic);
			if (c == '=') {
				lexer->lookahead = TOKEN_LE;
			} else if (c == '<') {
				c = input_getchar(osic);
				if (c == '=') {
					lexer->lookahead = TOKEN_SHL_ASSIGN;
				} else {
					input_ungetchar(osic, c);
					lexer->lookahead = TOKEN_SHL;
				}
			} else {
				input_ungetchar(osic, c);
				lexer->lookahead = TOKEN_LT;
			}
			break;

		case '&':
			c = input_getchar(osic);
			if (c == '&') {
				lexer->lookahead = TOKEN_LOGICAL_AND;
			} else if (c == '=') {
				lexer->lookahead = TOKEN_BITWISE_AND_ASSIGN;
			} else {
				input_ungetchar(osic, c);
				lexer->lookahead = TOKEN_BITWISE_AND;
			}
			break;

		case '^':
			c = input_getchar(osic);
			if (c == '=') {
				lexer->lookahead = TOKEN_BITWISE_XOR_ASSIGN;
			} else {
				input_ungetchar(osic, c);
				lexer->lookahead = TOKEN_BITWISE_XOR;
			}
			break;

		case '|':
			c = input_getchar(osic);
			if (c == '|') {
				lexer->lookahead = TOKEN_LOGICAL_OR;
			} else if (c == '=') {
				lexer->lookahead = TOKEN_BITWISE_OR_ASSIGN;
			} else {
				input_ungetchar(osic, c);
				lexer->lookahead = TOKEN_BITWISE_OR;
			}
			break;

		case '~':
			lexer->lookahead = TOKEN_BITWISE_NOT;
			break;

		case '?':
			lexer->lookahead = TOKEN_CONDITIONAL;
			break;

		case ':':
			lexer->lookahead = TOKEN_COLON;
			break;

		default:
			fprintf(stderr, "unknown token\n");
			lexer->lookahead = TOKEN_ERROR;
			return 0;
		}
	}

	return 1;
}
