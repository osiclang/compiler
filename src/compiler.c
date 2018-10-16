#include "osic.h"
#include "arena.h"
#include "input.h"
#include "lexer.h"
#include "scope.h"
#include "symbol.h"
#include "syntax.h"
#include "parser.h"
#include "operative_code.h"
#include "compiler.h"
#include "machine.h"
#include "generator.h"
#include "oModule.h"
#include "oNumber.h"
#include "oString.h"
#include "oInteger.h"

#include <ctype.h>
#include <assert.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#if !defined(STATICLIB) && !defined(WINDOWS)
#include <dlfcn.h>
#endif

#ifdef LINUX
#include <linux/limits.h>
#endif

#ifdef WINDOWS
#include <windows.h>
#include <Shlwapi.h>
#else
#include <unistd.h>
#endif

static int
compiler_expr(struct osic *osic, struct syntax *node);

static int
compiler_stmt(struct osic *osic, struct syntax *node);

static int
compiler_block(struct osic *osic, struct syntax *node);

static int
node_is_getter(struct osic *osic, struct syntax *node)
{
	return strncmp(node->u.accessor.name->buffer,
	               "getter",
	               (size_t)node->u.accessor.name->length) == 0;
}

static int
node_is_setter(struct osic *osic, struct syntax *node)
{
	return strncmp(node->u.accessor.name->buffer,
	               "setter",
	               (size_t)node->u.accessor.name->length) == 0;
}

static int
node_is_integer(struct osic *osic, struct syntax *node)
{
	int i;

	for (i = 0; i < node->length; i++) {
		if (node->buffer[i] == '.') {
			return 0;
		}
	}

	return 1;
}

static char *
resolve_module_name(struct osic *osic, char *path)
{
	int i;
	int c;

	char *name;
	char *first;
	char *last;

	name = arena_alloc(osic, osic->l_arena, OSIC_NAME_MAX + 1);

#ifdef WINDOWS
	first = strrchr(path, '\\');
#else
	first = strrchr(path, '/');
#endif
	if (!first) {
		first = path;
	} else {
		first += 1;
	}

	last = strrchr(path, '.');
	if (!last) {
		last = path + strlen(path);
	}

	for (i = 0; i < OSIC_NAME_MAX && first != last; i++) {
		c = *first++;
		if (isalnum(c)) {
			name[i] = (char)c;
		} else {
			name[i] = '_';
		}
	}
	name[i] = 0;
	if (isdigit(name[0])) {
		name[0] = '_';
	}

	return name;
}

static char *
resolve_module_path(struct osic *osic, struct syntax *node, char *path)
{
	char *first;
	char delimiter;
	char *resolved_name;
	char *resolved_path;

#ifdef WINDOWS
	if (path[0] != '.' && PathFileExists(path)) {
		return path;
	}
	delimiter = '\\';
#else
	if (path[0] != '.' && access(path, O_RDONLY) == 0) {
		return path;
	}
	delimiter = '/';
#endif
	if (path[0] != '.') {
#ifdef WINDOWS
		char environment[PATH_MAX];

		if (!GetEnvironmentVariable("OSIC_PATH",
		                            environment,
		                            PATH_MAX))
		{
			return path;
		}
#else
		char *environment;

		environment = getenv("OSIC_PATH");
		if (!environment) {
			return path;
		}
#endif
		resolved_path = arena_alloc(osic, osic->l_arena, PATH_MAX);
		memset(resolved_path, 0, PATH_MAX);
		snprintf(resolved_path,
		         PATH_MAX,
		         "%s%c%s",
		         environment,
		         delimiter,
		         path);
#ifdef WINDOWS
		if (!PathFileExists(resolved_path)) {
			return path;
		}
#else
		if (access(resolved_path, O_RDONLY) != 0) {
			return path;
		}
#endif

		return resolved_path;
	}

	resolved_name = arena_alloc(osic, osic->l_arena, PATH_MAX);
	memset(resolved_name, 0, PATH_MAX);

#ifdef WINDOWS
	if (!GetFullPathName(node->filename, PATH_MAX, resolved_name, NULL)) {
		return NULL;
	}
	resolved_path = resolved_name;
#else
	resolved_path = realpath(node->filename, resolved_name);
#endif
	if (!resolved_path) {
		return NULL;
	}

	first = strrchr(resolved_path, delimiter);
	assert(first);

	first[1] = '\0';
	strncat(resolved_path, path, PATH_MAX);

	return resolved_path;
}

#ifndef STATICLIB
static int
module_path_is_native(struct osic *osic, char *path)
{
	char *first;

	first = strrchr(path, '.');
	if (!first) {
		return 0;
	}

	if (strcmp(first, ".lm") == 0 || strcmp(first, ".osic") == 0) {
		return 0;
	}

	return 1;
}
#endif

static void
compiler_error(struct osic *osic, struct syntax *node, const char *message)
{
	fprintf(stderr,
	        "%s:%ld:%ld: error: %s",
	        node->filename,
	        node->line,
	        node->column,
	        message);
}

static void
compiler_error_undefined(struct osic *osic, struct syntax *node, char *name)
{
	char buffer[256];

	snprintf(buffer, sizeof(buffer), "undefined identifier '%s'\n", name);
	compiler_error(osic, node, buffer);
}

static void
compiler_error_redefined(struct osic *osic, struct syntax *node, char *name)
{
	char buffer[256];

	snprintf(buffer, sizeof(buffer), "redefined identifier '%s'\n", name);
	compiler_error(osic, node, buffer);
}

static int
compiler_const_object(struct osic *osic, struct oobject *object)
{
	int cpool;

	cpool = machine_add_const(osic, object);
	generator_emit_const(osic, cpool);

	return 1;
}

static int
compiler_const_string(struct osic *osic, char *buffer)
{
	struct oobject *object;

	object = ostring_create(osic, buffer, strlen(buffer));

	return compiler_const_object(osic, object);
}

static int
compiler_nil(struct osic *osic, struct syntax *node)
{
	if (!compiler_const_object(osic, osic->l_nil)) {
		return 0;
	}

	return 1;
}

static int
compiler_true(struct osic *osic, struct syntax *node)
{
	if (!compiler_const_object(osic, osic->l_true)) {
		return 0;
	}

	return 1;
}

static int
compiler_false(struct osic *osic, struct syntax *node)
{
	if (!compiler_const_object(osic, osic->l_false)) {
		return 0;
	}

	return 1;
}

static int
compiler_sentinel(struct osic *osic, struct syntax *node)
{
	if (!compiler_const_object(osic, osic->l_sentinel)) {
		return 0;
	}

	return 1;
}

static int
compiler_symbol(struct osic *osic, struct symbol *symbol)
{
	if (symbol->type == SYMBOL_LOCAL) {
		generator_emit_load(osic, symbol->level, symbol->local);
	} else if (symbol->type == SYMBOL_EXCEPTION) {
		generator_emit_operative_code(osic, OPERATIVE_CODE_LOADEXC);
	} else {
		generator_emit_const(osic, symbol->cpool);
	}

	return 1;
}

static int
compiler_array(struct osic *osic, struct syntax *node)
{
	int count;
	struct syntax *expr;

	count = 0;
	for (expr = node->u.array.expr_list; expr; expr = expr->sibling) {
		count += 1;
		if (!compiler_expr(osic, expr)) {
			return 0;
		}
	}
	generator_emit_array(osic, count);
	if (!osic->l_stmt_enclosing) {
		generator_emit_operative_code(osic, OPERATIVE_CODE_POP);
	}

	return 1;
}

static int
compiler_dictionary(struct osic *osic, struct syntax *node)
{
	int count;
	struct syntax *element;

	count = 0;
	for (element = node->u.dictionary.element_list;
	     element;
	     element = element->sibling)
	{
		count += 2;
		if (!compiler_expr(osic, element->u.element.value)) {
			return 0;
		}
		if (!compiler_expr(osic, element->u.element.name)) {
			return 0;
		}
	}
	generator_emit_dictionary(osic, count);
	if (!osic->l_stmt_enclosing) {
		generator_emit_operative_code(osic, OPERATIVE_CODE_POP);
	}

	return 1;
}

static int
compiler_unop(struct osic *osic, struct syntax *node)
{
	struct syntax *stmt_enclosing;

	stmt_enclosing = osic->l_stmt_enclosing;
	osic->l_stmt_enclosing = node;
	if (!compiler_expr(osic, node->u.unop.left)) {
		return 0;
	}
	osic->l_stmt_enclosing = stmt_enclosing;

	switch (node->opkind) {
	case SYNTAX_OPKIND_BITWISE_NOT:
		generator_emit_operative_code(osic, OPERATIVE_CODE_BNOT);
		break;

	case SYNTAX_OPKIND_LOGICAL_NOT:
		generator_emit_operative_code(osic, OPERATIVE_CODE_LNOT);
		break;

	case SYNTAX_OPKIND_POS:
		generator_emit_operative_code(osic, OPERATIVE_CODE_POS);
		break;

	case SYNTAX_OPKIND_NEG:
		generator_emit_operative_code(osic, OPERATIVE_CODE_NEG);
		break;

	default:
		return 0;
	}

	return 1;
}

static int
compiler_binop(struct osic *osic, struct syntax *node)
{
	struct generator_label *l_exit;

	struct syntax *stmt_enclosing;

	stmt_enclosing = osic->l_stmt_enclosing;
	osic->l_stmt_enclosing = node;

	if (node->opkind == SYNTAX_OPKIND_LOGICAL_OR) {
		/*
		 *    left_expr
		 *    dup
		 *    jnz l_exit
		 *    pop
		 *    right_expr
		 * l_exit:
		 */

		/*
		 * if (left_expr || right_expr)
		 * if the left_expr is true, the right_expr won't execute
		 */
		l_exit = generator_make_label(osic);

		if (!compiler_expr(osic, node->u.binop.left)) {
			return 0;
		}

		generator_emit_operative_code(osic, OPERATIVE_CODE_DUP);
		generator_emit_jnz(osic, l_exit);
		generator_emit_operative_code(osic, OPERATIVE_CODE_POP);

		if (!compiler_expr(osic, node->u.binop.right)) {
			return 0;
		}

		generator_emit_label(osic, l_exit);
	} else if (node->opkind == SYNTAX_OPKIND_LOGICAL_AND) {
		/*
		 *    left_expr
		 *    dup
		 *    jz l_exit
		 *    pop
		 *    right_expr
		 * l_exit:
		 */

		/*
		 * if (left_expr || right_expr)
		 * if the left_expr is false, the right_expr won't execute
		 */
		l_exit = generator_make_label(osic);

		if (!compiler_expr(osic, node->u.binop.left)) {
			return 0;
		}
		generator_emit_operative_code(osic, OPERATIVE_CODE_DUP);
		generator_emit_jz(osic, l_exit);
		generator_emit_operative_code(osic, OPERATIVE_CODE_POP);

		if (!compiler_expr(osic, node->u.binop.right)) {
			return 0;
		}

		generator_emit_label(osic, l_exit);
	} else {
		if (!compiler_expr(osic, node->u.binop.left)) {
			return 0;
		}

		if (!compiler_expr(osic, node->u.binop.right)) {
			return 0;
		}
		osic->l_stmt_enclosing = stmt_enclosing;

		switch (node->opkind) {
		case SYNTAX_OPKIND_ADD:
			generator_emit_operative_code(osic, OPERATIVE_CODE_ADD);
			break;

		case SYNTAX_OPKIND_SUB:
			generator_emit_operative_code(osic, OPERATIVE_CODE_SUB);
			break;

		case SYNTAX_OPKIND_MUL:
			generator_emit_operative_code(osic, OPERATIVE_CODE_MUL);
			break;

		case SYNTAX_OPKIND_DIV:
			generator_emit_operative_code(osic, OPERATIVE_CODE_DIV);
			break;

		case SYNTAX_OPKIND_MOD:
			generator_emit_operative_code(osic, OPERATIVE_CODE_MOD);
			break;

		case SYNTAX_OPKIND_SHL:
			generator_emit_operative_code(osic, OPERATIVE_CODE_SHL);
			break;

		case SYNTAX_OPKIND_SHR:
			generator_emit_operative_code(osic, OPERATIVE_CODE_SHR);
			break;

		case SYNTAX_OPKIND_GT:
			generator_emit_operative_code(osic, OPERATIVE_CODE_GT);
			break;

		case SYNTAX_OPKIND_GE:
			generator_emit_operative_code(osic, OPERATIVE_CODE_GE);
			break;

		case SYNTAX_OPKIND_LT:
			generator_emit_operative_code(osic, OPERATIVE_CODE_LT);
			break;

		case SYNTAX_OPKIND_LE:
			generator_emit_operative_code(osic, OPERATIVE_CODE_LE);
			break;

		case SYNTAX_OPKIND_EQ:
			generator_emit_operative_code(osic, OPERATIVE_CODE_EQ);
			break;

		case SYNTAX_OPKIND_NE:
			generator_emit_operative_code(osic, OPERATIVE_CODE_NE);
			break;

		case SYNTAX_OPKIND_IN:
			generator_emit_operative_code(osic, OPERATIVE_CODE_IN);
			break;

		case SYNTAX_OPKIND_BITWISE_AND:
			generator_emit_operative_code(osic, OPERATIVE_CODE_BAND);
			break;

		case SYNTAX_OPKIND_BITWISE_XOR:
			generator_emit_operative_code(osic, OPERATIVE_CODE_BXOR);
			break;

		case SYNTAX_OPKIND_BITWISE_OR:
			generator_emit_operative_code(osic, OPERATIVE_CODE_BOR);
			break;

		default:
			return 0;
		}
	}
	osic->l_stmt_enclosing = stmt_enclosing;

	return 1;
}

static int
compiler_conditional(struct osic *osic, struct syntax *node)
{
	/*
	 *    expr
	 *    jz l_false
	 *    true_expr
	 *    jmp l_exit
	 * l_false:
	 *    false_expr
	 * l_exit:
	 */

	/*
	 *    expr
	 *    dup
	 *    jnz l_exit
	 *    pop
	 *    false_expr
	 * l_exit:
	 */

	struct generator_label *l_false;
	struct generator_label *l_exit;

	l_exit = generator_make_label(osic);

	if (!compiler_expr(osic, node->u.conditional.expr)) {
		return 0;
	}

	/* if not set true_expr then set condition as true_expr */
	if (node->u.conditional.true_expr) {
		l_false = generator_make_label(osic);
		generator_emit_jz(osic, l_false);
		if (!compiler_expr(osic, node->u.conditional.true_expr)) {
			return 0;
		}
		generator_emit_jmp(osic, l_exit);

		generator_emit_label(osic, l_false);
		if (!compiler_expr(osic, node->u.conditional.false_expr)) {
			return 0;
		}
	} else {
		generator_emit_operative_code(osic, OPERATIVE_CODE_DUP);
		generator_emit_jnz(osic, l_exit);
		generator_emit_operative_code(osic, OPERATIVE_CODE_POP);
		if (!compiler_expr(osic, node->u.conditional.false_expr)) {
			return 0;
		}
	}

	generator_emit_label(osic, l_exit);

	return 1;
}

static int
compiler_argument(struct osic *osic, struct syntax *node)
{
	struct syntax *name;

	name = node->u.argument.name;
	if (node->argument_type == 0) {
		if (name) {
			if (!compiler_const_string(osic, name->buffer)) {
				return 0;
			}
			if (!compiler_expr(osic, node->u.argument.expr)) {
				return 0;
			}
			generator_emit_operative_code(osic, OPERATIVE_CODE_KARG);
		} else {
			if (!compiler_expr(osic, node->u.argument.expr)) {
				return 0;
			}
		}
	} else if (node->argument_type == 1) {
		if (!compiler_expr(osic, node->u.argument.expr)) {
			return 0;
		}
		generator_emit_operative_code(osic, OPERATIVE_CODE_VARG);
	} else if (node->argument_type == 2) {
		if (!compiler_expr(osic, node->u.argument.expr)) {
			return 0;
		}
		generator_emit_operative_code(osic, OPERATIVE_CODE_VKARG);
	}

	return 1;
}

static int
compiler_call(struct osic *osic, struct syntax *node)
{
	/*
	 *    callable
	 *    argument n
	 *    ...
	 *    argument 0
	 *    call n
	 */

	int argc;
	struct syntax *argument;
	struct syntax *stmt_enclosing;

	stmt_enclosing = osic->l_stmt_enclosing;
	osic->l_stmt_enclosing = node;

	if (!compiler_expr(osic, node->u.call.callable)) {
		return 0;
	}

	argc = 0;
	for (argument = node->u.call.argument_list;
	     argument;
	     argument = argument->sibling)
	{
		argc += 1;
		if (!compiler_argument(osic, argument)) {
			return 0;
		}
	}
	generator_emit_call(osic, argc);

	if (!stmt_enclosing) {
		/* dispose return value */
		generator_emit_operative_code(osic, OPERATIVE_CODE_POP);
	}
	osic->l_stmt_enclosing = stmt_enclosing;

	return 1;
}

static int
compiler_tailcall(struct osic *osic, struct syntax *node)
{
	int argc;
	struct syntax *argument;
	struct syntax *stmt_enclosing;

	stmt_enclosing = osic->l_stmt_enclosing;
	osic->l_stmt_enclosing = node;

	if (!compiler_expr(osic, node->u.call.callable)) {
		return 0;
	}

	argc = 0;
	for (argument = node->u.call.argument_list;
	     argument;
	     argument = argument->sibling)
	{
		argc += 1;
		if (!compiler_argument(osic, argument)) {
			return 0;
		}
	}
	generator_emit_tailcall(osic, argc);

	osic->l_stmt_enclosing = stmt_enclosing;

	return 1;
}

static int
compiler_name(struct osic *osic, struct syntax *node)
{
	int i;
	int len;

	struct symbol *symbol;
	struct syntax *accessor;
	struct syntax *accessors[128];
	struct syntax *stmt_enclosing;

	symbol = scope_get_symbol(osic, osic->l_scope, node->buffer);
	if (!symbol) {
		compiler_error_undefined(osic, node, node->buffer);

		return 0;
	}

	len = 0;
	for (accessor = symbol->accessor_list;
	     accessor;
	     accessor = accessor->sibling)
	{
		if (node_is_getter(osic, accessor)) {
			accessors[len++] = accessor;

			if (len == 128) {
				compiler_error(osic,
				               node,
				               "too many getters\n");

				return 0;
			}
		}
	}

	if (!len) {
		if (!compiler_symbol(osic, symbol)) {
			return 0;
		}
	}

	stmt_enclosing = osic->l_stmt_enclosing;
	osic->l_stmt_enclosing = node;
	for (i = len - 1; i >= 0; i--) {
		accessor = accessors[i];

		if (!compiler_expr(osic, accessor->u.accessor.expr)) {
			return 0;
		}

		if (i == 0) {
			if (!compiler_symbol(osic, symbol)) {
				return 0;
			}
		}
	}
	osic->l_stmt_enclosing = stmt_enclosing;

	for (i = 0; i < len; i++) {
		generator_emit_call(osic, 1);
	}

	return 1;
}

static int
compiler_store(struct osic *osic, struct syntax *node)
{
	struct syntax *accessor;
	struct symbol *symbol;

	if (node->kind == SYNTAX_KIND_NAME) {
		symbol = scope_get_symbol(osic,
		                          osic->l_scope,
		                          node->buffer);
		if (!symbol) {
			compiler_error_undefined(osic, node, node->buffer);

			return 0;
		}

		if (symbol->type != SYMBOL_LOCAL) {
			char buffer[256];

			snprintf(buffer,
			         sizeof(buffer),
			         "'%s' is not assignable\n",
			         symbol->name);
			compiler_error(osic, node, buffer);

			return 0;
		}

		for (accessor = symbol->accessor_list;
		     accessor;
		     accessor = accessor->sibling)
		{
			if (node_is_setter(osic, accessor)) {
				if (!compiler_expr(osic, accessor)) {
					return 0;
				}
				generator_emit_operative_code(osic, OPERATIVE_CODE_SWAP);
				generator_emit_call(osic, 1);
			}
		}

		generator_emit_store(osic, symbol->level, symbol->local);
	} else if (node->kind == SYNTAX_KIND_GET_ATTR) {
		char *buffer;

		if (!compiler_expr(osic, node->u.get_attr.left)) {
			return 0;
		}

		buffer = node->u.get_attr.right->buffer;
		if (!compiler_const_string(osic, buffer)) {
			return 0;
		}
		generator_emit_operative_code(osic, OPERATIVE_CODE_SETATTR);
	} else if (node->kind == SYNTAX_KIND_GET_ITEM) {
		compiler_expr(osic, node->u.get_item.left);
		compiler_expr(osic, node->u.get_item.right);
		generator_emit_operative_code(osic, OPERATIVE_CODE_SETITEM);
	} else {
		compiler_error(osic, node, "unsupport assignment\n");

		return 0;
	}

	return 1;
}

static int
compiler_assign_stmt(struct osic *osic, struct syntax *node)
{
	struct syntax *left;
	struct syntax *right;
	struct syntax *element;
	struct syntax *stmt_enclosing;

	stmt_enclosing = osic->l_stmt_enclosing;
	osic->l_stmt_enclosing = node;

	left = node->u.assign_stmt.left;
	right = node->u.assign_stmt.right;

	if (node->opkind != SYNTAX_OPKIND_ASSIGN) {
		if (left->kind == SYNTAX_KIND_NAME ||
		    left->kind == SYNTAX_KIND_GET_ATTR ||
		    left->kind == SYNTAX_KIND_GET_ITEM)
		{
			if (!compiler_expr(osic, left)) {
				return 0;
			}
		} else {
			compiler_error(osic, left, "unsupport operator\n");

			return 0;
		}
	}

	if (!compiler_expr(osic, right)) {
		return 0;
	}

	switch (node->opkind) {
	case SYNTAX_OPKIND_ASSIGN:
		/* do nothing */
		break;

	case SYNTAX_OPKIND_ADD_ASSIGN:
		generator_emit_operative_code(osic, OPERATIVE_CODE_ADD);
		break;

	case SYNTAX_OPKIND_SUB_ASSIGN:
		generator_emit_operative_code(osic, OPERATIVE_CODE_SUB);
		break;

	case SYNTAX_OPKIND_MUL_ASSIGN:
		generator_emit_operative_code(osic, OPERATIVE_CODE_MUL);
		break;

	case SYNTAX_OPKIND_DIV_ASSIGN:
		generator_emit_operative_code(osic, OPERATIVE_CODE_DIV);
		break;

	case SYNTAX_OPKIND_MOD_ASSIGN:
		generator_emit_operative_code(osic, OPERATIVE_CODE_MOD);
		break;

	case SYNTAX_OPKIND_SHL_ASSIGN:
		generator_emit_operative_code(osic, OPERATIVE_CODE_SHL);
		break;

	case SYNTAX_OPKIND_SHR_ASSIGN:
		generator_emit_operative_code(osic, OPERATIVE_CODE_SHR);
		break;

	case SYNTAX_OPKIND_BITWISE_AND_ASSIGN:
		generator_emit_operative_code(osic, OPERATIVE_CODE_BAND);
		break;

	case SYNTAX_OPKIND_BITWISE_XOR_ASSIGN:
		generator_emit_operative_code(osic, OPERATIVE_CODE_BXOR);
		break;

	case SYNTAX_OPKIND_BITWISE_OR_ASSIGN:
		generator_emit_operative_code(osic, OPERATIVE_CODE_BOR);
		break;

	default:
		compiler_error(osic, left, "unsupport operator\n");
		return 0;
	}

	if (left->kind == SYNTAX_KIND_UNPACK) {
		int unpack;

		unpack = 0;
		for (element = left->u.unpack.expr_list;
		     element;
		     element = element->sibling)
		{
			unpack += 1;
		}

		generator_emit_unpack(osic, unpack);
		for (element = left->u.unpack.expr_list;
		     element;
		     element = element->sibling)
		{
			if (!compiler_store(osic, element)) {
				return 0;
			}
		}
	} else {
		if (!compiler_store(osic, left)) {
			return 0;
		}
	}
	osic->l_stmt_enclosing = stmt_enclosing;

	return 1;
}

static int
compiler_var_stmt(struct osic *osic, struct syntax *node)
{
	struct symbol *symbol;
	struct syntax *stmt_enclosing;
	struct syntax *space_enclosing;

	stmt_enclosing = osic->l_stmt_enclosing;
	osic->l_stmt_enclosing = node;

	if (node->u.var_stmt.expr) {
		if (!compiler_expr(osic, node->u.var_stmt.expr)) {
			return 0;
		}
	}

	symbol = scope_add_symbol(osic,
	                          osic->l_scope,
	                          node->u.var_stmt.name->buffer,
	                          SYMBOL_LOCAL);
	if (!symbol) {
		compiler_error_redefined(osic,
		                         node->u.var_stmt.name,
		                         node->u.var_stmt.name->buffer);

		return 0;
	}

	space_enclosing = osic->l_space_enclosing;
	symbol->accessor_list = node->u.var_stmt.accessor_list;
	symbol->local = space_enclosing->nlocals++;

	if (node->u.var_stmt.expr) {
		generator_emit_store(osic, 0, symbol->local);
	}

	osic->l_stmt_enclosing = stmt_enclosing;

	return 1;
}

static int
compiler_if_stmt(struct osic *osic, struct syntax *node)
{
	/*
	 *    expr
	 *    jz l_else
	 *    then_block_stmt
	 *    jmp l_exit
	 * l_else:
	 *    else_block_stmt
	 * l_exit:
	 */

	struct generator_label *l_else;
	struct generator_label *l_exit;
	struct syntax *stmt_enclosing;

	stmt_enclosing = osic->l_stmt_enclosing;
	osic->l_stmt_enclosing = node;
	if (!compiler_expr(osic, node->u.if_stmt.expr)) {
		return 0;
	}
	osic->l_stmt_enclosing = stmt_enclosing;

	l_else = generator_make_label(osic);
	l_exit = generator_make_label(osic);

	generator_emit_jz(osic, l_else);
	if (!compiler_block(osic, node->u.if_stmt.then_block_stmt)) {
		return 0;
	}
	if (node->u.if_stmt.else_block_stmt) {
		generator_emit_jmp(osic, l_exit);
		generator_emit_label(osic, l_else);
		if (!compiler_stmt(osic, node->u.if_stmt.else_block_stmt)) {
			return 0;
		}
	} else {
		generator_emit_label(osic, l_else);
	}
	generator_emit_label(osic, l_exit);

	return 1;
}

static int
compiler_for_stmt(struct osic *osic, struct syntax *node)
{
	/*
	 *    init_expr
	 * l_cond:
	 *    cond_expr
	 *    jz l_exit
	 *    block_stmt
	 * l_step:          ; l_continue
	 *    step_expr
	 *    jmp l_expr
	 * l_exit:
	 */

	struct generator_label *l_cond;
	struct generator_label *l_step;
	struct generator_label *l_exit;

	struct syntax *init_expr;
	struct syntax *step_expr;
	struct syntax *stmt_enclosing;
	struct syntax *loop_enclosing;

	loop_enclosing = osic->l_loop_enclosing;
	osic->l_loop_enclosing = node;

	l_cond = generator_make_label(osic);
	l_step = generator_make_label(osic);
	l_exit = generator_make_label(osic);

	stmt_enclosing = osic->l_stmt_enclosing;
	osic->l_stmt_enclosing = node;

	init_expr = node->u.for_stmt.init_expr;
	if (init_expr->kind == SYNTAX_KIND_VAR_STMT) {
		if (!compiler_var_stmt(osic, init_expr)) {
			return 0;
		}
	} else if (init_expr->kind == SYNTAX_KIND_ASSIGN_STMT) {
		if (!compiler_assign_stmt(osic, init_expr)) {
			return 0;
		}
	} else {
		if (!compiler_expr(osic, init_expr)) {
			return 0;
		}
	}

	generator_emit_label(osic, l_cond);
	if (!compiler_expr(osic, node->u.for_stmt.cond_expr)) {
		return 0;
	}
	osic->l_stmt_enclosing = stmt_enclosing;
	generator_emit_jz(osic, l_exit);

	node->l_break = l_exit;
	node->l_continue = l_step;
	if (!compiler_block(osic, node->u.for_stmt.block_stmt)) {
		return 0;
	}

	stmt_enclosing = osic->l_stmt_enclosing;
	osic->l_stmt_enclosing = node;
	generator_emit_label(osic, l_step);
	step_expr = node->u.for_stmt.step_expr;
	if (step_expr->kind == SYNTAX_KIND_ASSIGN_STMT) {
		if (!compiler_assign_stmt(osic, step_expr)) {
			return 0;
		}
	} else {
		if (!compiler_expr(osic, step_expr)) {
			return 0;
		}
	}
	osic->l_stmt_enclosing = stmt_enclosing;
	generator_emit_jmp(osic, l_cond);
	generator_emit_label(osic, l_exit);
	osic->l_loop_enclosing = loop_enclosing;

	return 1;
}

static int
compiler_forin_stmt(struct osic *osic, struct syntax *node)
{
	/*
	 *    iter
	 *    const "__iterator__"
	 *    getattr
	 *    call 0
	 *    store local
	 * l_next:
	 *    store local
	 *    const "__next__"
	 *    getattr
	 *    call 0
	 *    dup
	 *    store name
	 *    const sentinal
	 *    eq
	 *    jnz l_exit
	 *    step_expr
	 *    block_stmt
	 *    jmp l_next
	 * l_exit:
	 */

	int local;

	struct generator_label *l_next;
	struct generator_label *l_exit;

	struct syntax *name;
	struct syntax *stmt_enclosing;
	struct syntax *loop_enclosing;
	struct syntax *space_enclosing;
	struct symbol *symbol;

	space_enclosing = osic->l_space_enclosing;
	local = space_enclosing->nlocals++;

	loop_enclosing = osic->l_loop_enclosing;
	osic->l_loop_enclosing = node;

	scope_enter(osic, SCOPE_BLOCK);

	l_next = generator_make_label(osic);
	l_exit = generator_make_label(osic);

	stmt_enclosing = osic->l_stmt_enclosing;
	osic->l_stmt_enclosing = node;
	if (!compiler_expr(osic, node->u.forin_stmt.iter)) {
		return 0;
	}

	if (!compiler_const_object(osic, osic->l_iterator_string)) {
		return 0;
	}
	osic->l_stmt_enclosing = stmt_enclosing;
	generator_emit_operative_code(osic, OPERATIVE_CODE_GETATTR);
	generator_emit_call(osic, 0);
	generator_emit_store(osic, 0, local);

	generator_emit_label(osic, l_next);

	generator_emit_load(osic, 0, local);
	if (!compiler_const_object(osic, osic->l_next_string)) {
		return 0;
	}
	generator_emit_operative_code(osic, OPERATIVE_CODE_GETATTR);
	generator_emit_call(osic, 0);
	generator_emit_operative_code(osic, OPERATIVE_CODE_DUP); /* store and cmp */

	name = node->u.forin_stmt.name;
	if (name->kind == SYNTAX_KIND_VAR_STMT) {
		name = name->u.var_stmt.name;
		symbol = scope_add_symbol(osic,
		                          osic->l_scope,
		                          name->buffer,
		                          SYMBOL_LOCAL);
		if (!symbol) {
			compiler_error_redefined(osic, name, name->buffer);

			return 0;
		}
		symbol->local = space_enclosing->nlocals++;
	} else {
		symbol = scope_get_symbol(osic, osic->l_scope, name->buffer);
		if (!symbol) {
			compiler_error_undefined(osic,
			                         name, name->buffer);

			return 0;
		}
	}
	generator_emit_store(osic, symbol->level, symbol->local);

	if (!compiler_const_object(osic, osic->l_sentinel)) {
		return 0;
	}
	generator_emit_operative_code(osic, OPERATIVE_CODE_EQ);
	generator_emit_jnz(osic, l_exit);

	node->l_break = l_exit;
	node->l_continue = l_next;

	if (!compiler_block(osic, node->u.forin_stmt.block_stmt)) {
		return 0;
	}
	generator_emit_jmp(osic, l_next);
	generator_emit_label(osic, l_exit);

	osic->l_loop_enclosing = loop_enclosing;
	scope_leave(osic);

	return 1;
}

static int
compiler_while_stmt(struct osic *osic, struct syntax *node)
{
	/*
	 * l_cond:
	 *    expr
	 *    jz l_exit
	 *    block_stmt
	 *    jmp l_cond
	 * l_exit:
	 */

	struct generator_label *l_cond;
	struct generator_label *l_exit;

	struct syntax *stmt_enclosing;
	struct syntax *loop_enclosing;

	loop_enclosing = osic->l_loop_enclosing;
	osic->l_loop_enclosing = node;

	l_cond = generator_make_label(osic);
	l_exit = generator_make_label(osic);

	node->l_break = l_exit;
	node->l_continue = l_cond;

	stmt_enclosing = osic->l_stmt_enclosing;
	osic->l_stmt_enclosing = node;
	generator_emit_label(osic, l_cond);
	if (!compiler_expr(osic, node->u.while_stmt.expr)) {
		return 0;
	}
	osic->l_stmt_enclosing = stmt_enclosing;
	generator_emit_jz(osic, l_exit);

	if (!compiler_block(osic, node->u.while_stmt.block_stmt)) {
		return 0;
	}
	generator_emit_jmp(osic, l_cond);

	generator_emit_label(osic, l_exit);
	osic->l_loop_enclosing = loop_enclosing;

	return 1;
}

static int
compiler_break_stmt(struct osic *osic, struct syntax *node)
{
	struct syntax *loop_enclosing;

	loop_enclosing = osic->l_loop_enclosing;
	if (!loop_enclosing) {
		compiler_error(osic, node, "'break' statement not in loop\n");

		return 0;
	}
	generator_emit_jmp(osic, loop_enclosing->l_break);

	return 1;
}

static int
compiler_continue_stmt(struct osic *osic, struct syntax *node)
{
	struct syntax *loop_enclosing;

	loop_enclosing = osic->l_loop_enclosing;
	if (!loop_enclosing) {
		compiler_error(osic,
		               node,
		               "'continue' statement not in loop\n");

		return 0;
	}
	generator_emit_jmp(osic, loop_enclosing->l_continue);

	return 1;
}

static int
compiler_parameter(struct osic *osic, struct syntax *node)
{
	/*
	 *    const sentinel
	 *    load parameter
	 *    eq
	 *    jz l_exit
	 *    expr
	 *    store parameter
	 * l_exit:
	 */

	struct generator_label *l_exit;

	struct syntax *accessor;
	struct syntax *stmt_enclosing;
	struct syntax *space_enclosing;

	struct symbol *symbol;

	space_enclosing = osic->l_space_enclosing;
	if (space_enclosing->kind != SYNTAX_KIND_DEFINE_STMT) {
		return 0;
	}
	space_enclosing->nparams++;

	symbol = scope_add_symbol(osic,
	                          osic->l_scope,
	                          node->u.parameter.name->buffer,
	                          SYMBOL_LOCAL);
	if (!symbol) {
		compiler_error_redefined(osic,
		                         node->u.parameter.name,
		                         node->u.parameter.name->buffer);

		return 0;
	}
	symbol->local = space_enclosing->nlocals++;
	symbol->accessor_list = node->u.parameter.accessor_list;

	if (node->u.parameter.expr) {
		space_enclosing->nvalues++;

		l_exit = generator_make_label(osic);

		/* make compare with sentinel(unset) */
		if (!compiler_const_object(osic, osic->l_sentinel)) {
			return 0;
		}
		generator_emit_load(osic, 0, symbol->local);
		generator_emit_operative_code(osic, OPERATIVE_CODE_EQ);
		generator_emit_jz(osic, l_exit);

		stmt_enclosing = osic->l_stmt_enclosing;
		osic->l_stmt_enclosing = node;
		if (!compiler_expr(osic, node->u.parameter.expr)) {
			return 0;
		}
		osic->l_stmt_enclosing = stmt_enclosing;

		generator_emit_store(osic, 0, symbol->local);
		generator_emit_label(osic, l_exit);

	}

	if (symbol->accessor_list) {
		generator_emit_load(osic, symbol->level, symbol->local);
		for (accessor = symbol->accessor_list;
		     accessor;
		     accessor = accessor->sibling)
		{
			if (node_is_setter(osic, accessor)) {
				if (!compiler_expr(osic, accessor)) {
					return 0;
				}
				generator_emit_operative_code(osic, OPERATIVE_CODE_SWAP);
				generator_emit_call(osic, 1);
			}
		}
	}

	return 1;
}

static int
compiler_define_stmt(struct osic *osic, struct syntax *node)
{
	/*
	 *    const parameter_name_0
	 *    ...
	 *    const parameter_name_n
	 *
	 *    const function_name
	 *    define define, nvalues, nparams, nlocals, l_exit
	 *    parameter_0
	 *    ...
	 *    parameter_n
	 *    block_stmt
	 *    const sentinel
	 *    return
	 * l_exit:
	 */

	int define;
	struct generator_label *l_exit;
	struct generator_code *c_define;

	struct scope *scope;
	struct symbol *symbol;

	struct syntax *name;
	struct syntax *parameter;

	struct syntax *try_enclosing;
	struct syntax *stmt_enclosing;
	struct syntax *space_enclosing;

	l_exit = generator_make_label(osic);

	/* generate default parameters value */
	for (parameter = node->u.define_stmt.parameter_list;
	     parameter;
	     parameter = parameter->sibling)
	{
		name = parameter->u.parameter.name;
		if (!compiler_const_string(osic, name->buffer)) {
			return 0;
		}
	}

	space_enclosing = osic->l_space_enclosing;
	osic->l_space_enclosing = node;

	scope = osic->l_scope;
	scope_enter(osic, SCOPE_DEFINE);

	name = node->u.define_stmt.name;
	if (name) {
		symbol = scope_add_symbol(osic,
		                          scope,
		                          name->buffer,
		                          SYMBOL_LOCAL);
		if (!symbol) {
			compiler_error_redefined(osic, name, name->buffer);

			return 0;
		}
		symbol->accessor_list = node->u.define_stmt.accessor_list;
		if (!compiler_const_string(osic, name->buffer)) {
			return 0;
		}
		symbol->local = space_enclosing->nlocals++;;
	} else {
		if (!compiler_const_string(osic, "")) {
			return 0;
		}
		symbol = NULL;
	}

	/* 'define' will create a function object and jmp function end */
	c_define = generator_emit_define(osic, 0, 0, 0, 0, 0);

	try_enclosing = osic->l_try_enclosing;
	osic->l_try_enclosing = NULL;

	define = 0;
	/* generate function parameters name(for call keyword arguments) */
	for (parameter = node->u.define_stmt.parameter_list;
	     parameter;
	     parameter = parameter->sibling)
	{
		if (parameter->parameter_type == 0 && define == 0) {
			/* define func(var a, var b, var c) */
			define = 0;
		} else if (parameter->parameter_type == 1 && define == 0) {
			/* define func(var a, var b, var *c) */
			define = 1;
		} else if (parameter->parameter_type == 2 && define == 0) {
			/* define func(var a, var b, var **c) */
			define = 2;
		} else if (parameter->parameter_type == 2 && define == 1) {
			/* define func(var a, var *b, var **c) */
			define = 3;
		} else {
			compiler_error(osic, node, "wrong parameter type\n");

			return 0;
		}

		if (!compiler_parameter(osic, parameter)) {
			return 0;
		}
	}

	stmt_enclosing = osic->l_stmt_enclosing;
	osic->l_stmt_enclosing = NULL;

	if (!compiler_block(osic, node->u.define_stmt.block_stmt)) {
		return 0;
	}

	if (!node->u.define_stmt.block_stmt->has_return) {
		if (!compiler_const_object(osic, osic->l_nil)) {
			return 0;
		}
		generator_emit_operative_code(osic, OPERATIVE_CODE_RETURN);
	}

	generator_emit_label(osic, l_exit);

	/* patch define */
	generator_patch_define(osic,
	                       c_define,
	                       define,
	                       node->nvalues,
	                       node->nparams,
	                       node->nlocals,
	                       l_exit);

	/* if define in class don't emit store */
	if (scope && scope->type != SCOPE_CLASS) {
		if (name) {
			/*
			 * 'store' will pop 'define' function object
			 * so if we're in assignment or call stmt,
			 * 'dup' function for assignment or call
			 * like 'var foo = define func(){};'
			 * or 'define func(){}();'
			 */
			if (stmt_enclosing) {
				generator_emit_operative_code(osic, OPERATIVE_CODE_DUP);
			}
			generator_emit_store(osic, 0, symbol->local);
		}
	}

	scope_leave(osic);
	osic->l_try_enclosing = try_enclosing;
	osic->l_stmt_enclosing = stmt_enclosing;
	osic->l_space_enclosing = space_enclosing;

	return 1;
}

static int
compiler_delete_stmt(struct osic *osic, struct syntax *node)
{
	struct syntax *expr;
	struct syntax *stmt_enclosing;

	stmt_enclosing = osic->l_stmt_enclosing;
	osic->l_stmt_enclosing = node;
	if (!node->u.delete_stmt.expr) {
		return 0;
	}

	expr = node->u.return_stmt.expr;
	if (expr->kind == SYNTAX_KIND_GET_ITEM) {
		if (!compiler_expr(osic, expr->u.get_item.left)) {
			return 0;
		}
		if (!compiler_expr(osic, expr->u.get_item.right)) {
			return 0;
		}
		generator_emit_operative_code(osic, OPERATIVE_CODE_DELITEM);
	} else if (expr->kind == SYNTAX_KIND_GET_ATTR) {
		if (!compiler_expr(osic, expr->u.get_attr.left)) {
			return 0;
		}
		if (!compiler_const_string(osic,
		                           expr->u.get_attr.right->buffer))
		{
			return 0;
		}
		generator_emit_operative_code(osic, OPERATIVE_CODE_DELATTR);
	} else {
		compiler_error(osic, node, "unsupport delete stmt\n");

		return 0;
	}
	osic->l_stmt_enclosing = stmt_enclosing;

	return 1;
}

static int
compiler_return_stmt(struct osic *osic, struct syntax *node)
{
	struct syntax *try_enclosing;
	struct syntax *stmt_enclosing;
	struct syntax *space_enclosing;
	struct syntax *block_stmt;

	space_enclosing = osic->l_space_enclosing;
	if (space_enclosing->kind != SYNTAX_KIND_DEFINE_STMT) {
		compiler_error(osic,
		               node,
		               "'return' statement not in function\n");

		return 0;
	}

	try_enclosing = osic->l_try_enclosing;
	while (try_enclosing) {
		block_stmt = try_enclosing->u.try_stmt.finally_block_stmt;
		if (block_stmt && !compiler_block(osic, block_stmt)) {
			return 0;
		}
		try_enclosing = try_enclosing->parent;
	}

	stmt_enclosing = osic->l_stmt_enclosing;
	osic->l_stmt_enclosing = node;
	if (node->u.return_stmt.expr) {
		struct syntax *expr;

		expr = node->u.return_stmt.expr;
		if (expr->kind == SYNTAX_KIND_CALL) {
			if (!compiler_tailcall(osic, expr)) {
				return 0;
			}
		} else {
			if (!compiler_expr(osic, expr)) {
				return 0;
			}
		}
	} else {
		if (!compiler_const_object(osic, osic->l_nil)) {
			return 0;
		}
	}
	node->has_return = 1;
	generator_emit_operative_code(osic, OPERATIVE_CODE_RETURN);
	osic->l_stmt_enclosing = stmt_enclosing;

	/* remove all code after return stmt */
	node->sibling = NULL;

	return 1;
}

static int
compiler_class_stmt(struct osic *osic, struct syntax *node)
{
	/*
	 *    attr_value
	 *    attr_name
	 *    attr_value
	 *    attr_name
	 *    super_n
	 *    ...
	 *    super_1
	 *    class nsupers, nattrs
	 */

	int local;
	int nattrs;
	int nsupers;
	char *class_name;

	struct syntax *name;
	struct syntax *stmt;
	struct syntax *block;
	struct syntax *super_expr;
	struct syntax *stmt_enclosing;
	struct syntax *space_enclosing;
	struct symbol *symbol;

	space_enclosing = osic->l_space_enclosing;
	local = space_enclosing->nlocals++;

	name = node->u.class_stmt.name;
	if (name) {
		symbol = scope_add_symbol(osic,
		                          osic->l_scope,
		                          name->buffer,
		                          SYMBOL_LOCAL);
		if (!symbol) {
			compiler_error_redefined(osic, name, name->buffer);

			return 0;
		}
		symbol->local = local;
		symbol->accessor_list = node->u.class_stmt.accessor_list;
		class_name = name->buffer;
	} else {
		class_name = "";
	}

	scope_enter(osic, SCOPE_CLASS);
	/* generate name and value pair */
	nattrs = 0;
	block = node->u.class_stmt.block_stmt;
	for (stmt = block->u.block_stmt.stmt_list;
	     stmt;
	     stmt = stmt->sibling)
	{
		nattrs += 2;
		if (stmt->kind == SYNTAX_KIND_VAR_STMT) {
			if (stmt->u.var_stmt.expr) {
				stmt_enclosing = osic->l_stmt_enclosing;
				osic->l_stmt_enclosing = stmt;
				if (!compiler_expr(osic,
				                   stmt->u.var_stmt.expr))
				{
					return 0;
				}
				osic->l_stmt_enclosing = stmt_enclosing;
			} else {
				if (!compiler_const_object(osic,
				                           osic->l_nil))
				{
					return 0;
				}
			}
			name = stmt->u.var_stmt.name;
		} else if (stmt->kind == SYNTAX_KIND_DEFINE_STMT) {
			if (!compiler_define_stmt(osic, stmt)) {
				return 0;
			}
			name = stmt->u.define_stmt.name;
		} else {
			compiler_error(osic,
			               node,
			               "class define only support "
			               "`define' and `var'\n");

			return 0;
		}

		if (!compiler_const_string(osic, name->buffer)) {
			return 0;
		}
	}
	scope_leave(osic);

	/* generate super list */
	nsupers = 0;
	for (super_expr = node->u.class_stmt.super_list;
	     super_expr;
	     super_expr = super_expr->sibling)
	{
		nsupers += 1;
		if (!compiler_expr(osic, super_expr)) {
			return 0;
		}
	}

	if (!compiler_const_string(osic, class_name)) {
		return 0;
	}

	generator_emit_class(osic, nsupers, nattrs);

	/*
	 * 'store' will pop 'class' object
	 * so if we're in assignment or call stmt,
	 * 'dup' object for assignment or call
	 * like 'var foo = class bar{};' or 'class foo{}();'
	 */
	stmt_enclosing = osic->l_stmt_enclosing;
	if (stmt_enclosing) {
		generator_emit_operative_code(osic, OPERATIVE_CODE_DUP);
	}

	generator_emit_store(osic, 0, local);

	/* generate class getter and setters */
	block = node->u.class_stmt.block_stmt;
	for (stmt = block->u.block_stmt.stmt_list;
	     stmt;
	     stmt = stmt->sibling)
	{
		int n;
		struct syntax *accessor;
		struct syntax *accessor_list;

		if (stmt->kind == SYNTAX_KIND_VAR_STMT) {
			name = stmt->u.var_stmt.name;
			accessor_list = stmt->u.var_stmt.accessor_list;
		} else if (stmt->kind == SYNTAX_KIND_CLASS_STMT) {
			name = stmt->u.class_stmt.name;
			accessor_list = stmt->u.class_stmt.accessor_list;
		} else if (stmt->kind == SYNTAX_KIND_DEFINE_STMT) {
			name = stmt->u.define_stmt.name;
			accessor_list = stmt->u.define_stmt.accessor_list;
		} else {
			return 0;
		}

		if (accessor_list) {
			osic->l_stmt_enclosing = stmt;

			n = 0;
			for (accessor = accessor_list;
			     accessor;
			     accessor = accessor->sibling)
			{
				if (node_is_getter(osic, accessor)) {
					if (!compiler_expr(osic, accessor)) {
						return 0;
					}
					n += 1;
				}
			}
			if (n) {
				generator_emit_load(osic, 0, local);
				if (!compiler_const_string(osic,
				                           name->buffer))
				{
					return 0;
				}
				generator_emit_setgetter(osic, n);
			}

			n = 0;
			for (accessor = accessor_list;
			     accessor;
			     accessor = accessor->sibling)
			{
				if (node_is_setter(osic, accessor)) {
					if (!compiler_expr(osic, accessor)) {
						return 0;
					}
					n += 1;
				}
			}

			if (n) {
				generator_emit_load(osic, 0, local);
				if (!compiler_const_string(osic,
				                           name->buffer))
				{
					return 0;
				}
				generator_emit_setsetter(osic, n);
			}
			osic->l_stmt_enclosing = stmt_enclosing;
		}
	}

	return 1;
}

static int
compiler_try_stmt(struct osic *osic, struct syntax *node)
{
	/* try */
	/*
	 *    try: l_catch
	 *    try_block_stmt
	 * l_catched_finally:
	 *    finally_block_stmt
	 *    jmp l_exit
	 * l_catch:
	 *    ...
	 *    catch_stmt
	 *    ...
	 *    finally_block_stmt
	 *    untry
	 *    loadexc
	 *    throw
	 */

	/* catch */
	/*
	 *    loadexc
	 *    const "__instanceof__"
	 *    getattr
	 *    catch_type
	 *    call 1           ; exception.__instanceof__(catch_type)
	 *    jz l_catch_exit
	 *    block_stmt
	 *    jmp l_catched_finally
	 * l_catch_exit:
	 */

	struct generator_label *l_exit;
	struct generator_label *l_catch;
	struct generator_label *l_catch_exit;

	/* catched finally or unthrow finally */
	struct generator_label *l_catched_finally;

	struct syntax *catch_stmt;
	struct syntax *try_enclosing;
	struct syntax *finally_block_stmt;

	node->parent = osic->l_try_enclosing;
	try_enclosing = osic->l_try_enclosing;
	osic->l_try_enclosing = node;

	l_exit = generator_make_label(osic);
	l_catch = generator_make_label(osic);
	l_catched_finally = generator_make_label(osic);

	generator_emit_try(osic, l_catch);
	if (!compiler_block(osic, node->u.try_stmt.try_block_stmt)) {
		return 0;
	}

	generator_emit_label(osic, l_catched_finally);

	finally_block_stmt = node->u.try_stmt.finally_block_stmt;
	if (finally_block_stmt) {
		/*
		 * Don't want current try_stmt if
		 * finally_block_stmt has return_stmt
		 */
		osic->l_try_enclosing = node->parent;
		if (!compiler_block(osic, finally_block_stmt)) {
			return 0;
		}
		osic->l_try_enclosing = node;
	}
	generator_emit_operative_code(osic, OPERATIVE_CODE_UNTRY);
	generator_emit_jmp(osic, l_exit);

	generator_emit_label(osic, l_catch);
	node->l_catch = l_catch;
	for (catch_stmt = node->u.try_stmt.catch_stmt_list;
	     catch_stmt;
	     catch_stmt = catch_stmt->sibling)
	{
		struct syntax *catch_name;
		struct symbol *symbol;

		l_catch_exit = generator_make_label(osic);

		/* compare exception class */
		generator_emit_operative_code(osic, OPERATIVE_CODE_LOADEXC);
		if (!compiler_const_string(osic, "__instanceof__")) {
			return 0;
		}
		generator_emit_operative_code(osic, OPERATIVE_CODE_GETATTR);
		if (!compiler_expr(osic,
		                   catch_stmt->u.catch_stmt.catch_type))
		{
			return 0;
		}
		generator_emit_call(osic, 1);
		generator_emit_jz(osic, l_catch_exit);

		scope_enter(osic, SCOPE_BLOCK);
		catch_name = catch_stmt->u.catch_stmt.catch_name;
		symbol = scope_add_symbol(osic,
		                          osic->l_scope,
		                          catch_name->buffer,
		                          SYMBOL_EXCEPTION);
		if (!symbol) {
			compiler_error_redefined(osic,
			                         catch_name,
			                         catch_name->buffer);

			return 0;
		}
		if (!compiler_block(osic,
		                    catch_stmt->u.catch_stmt.block_stmt))
		{
			return 0;
		}
		scope_leave(osic);
		generator_emit_jmp(osic, l_catched_finally);

		generator_emit_label(osic, l_catch_exit);
	}

	/* rethrow finally */
	if (finally_block_stmt) {
		osic->l_try_enclosing = node->parent;
		if (!compiler_block(osic, finally_block_stmt)) {
			return 0;
		}
		osic->l_try_enclosing = node;
	}
	if (try_enclosing && try_enclosing->l_catch) {
		/* upper level catch block */
		generator_emit_jmp(osic, try_enclosing->l_catch);
	} else {
		/* top level try catch block, rethrow */
		generator_emit_operative_code(osic, OPERATIVE_CODE_UNTRY);
		generator_emit_operative_code(osic, OPERATIVE_CODE_LOADEXC);
		generator_emit_operative_code(osic, OPERATIVE_CODE_THROW);
	}

	generator_emit_label(osic, l_exit);
	osic->l_try_enclosing = try_enclosing;

	return 1;
}

static int
compiler_throw_stmt(struct osic *osic, struct syntax *node)
{
	struct syntax *stmt_enclosing;

	stmt_enclosing = osic->l_stmt_enclosing;
	osic->l_stmt_enclosing = node;
	if (!compiler_expr(osic, node->u.return_stmt.expr)) {
		return 0;
	}
	generator_emit_operative_code(osic, OPERATIVE_CODE_THROW);
	/* remove all code after throw stmt */
	node->sibling = NULL;
	node->has_return = 1;
	osic->l_stmt_enclosing = stmt_enclosing;

	return 1;
}

static int
compiler_block(struct osic *osic, struct syntax *node)
{
	struct syntax *stmt;

	scope_enter(osic, SCOPE_BLOCK);
	for (stmt = node->u.block_stmt.stmt_list;
	     stmt;
	     stmt = stmt->sibling)
	{
		if (!compiler_stmt(osic, stmt)) {
			return 0;
		}

		if (stmt->has_return) {
			node->has_return = 1;
		}
	}
	scope_leave(osic);

	return 1;
}

static int
compiler_module(struct osic *osic, struct syntax *node)
{
	struct generator_label *l_exit;
	struct generator_code *c_module;

	char *module_name;
	char *module_path;
	struct scope *module_scope;
	struct oobject *module_key;
	struct oobject *module;

	struct syntax *stmt;
	struct syntax *stmt_enclosing;
	struct syntax *space_enclosing;
	struct symbol *symbol;

	scope_enter(osic, SCOPE_MODULE);
	l_exit = generator_make_label(osic);

	module_name = resolve_module_name(osic, node->filename);
	module = omodule_create(osic,
	                        ostring_create(osic,
	                                       module_name,
	                                       strlen(module_name)));
	if (!module) {
		return 0;
	}
	module_path = resolve_module_path(osic, node, input_filename(osic));
	module_key = ostring_create(osic, module_path, strlen(module_path));
	oobject_set_item(osic, osic->l_modules, module_key, module);

	compiler_const_object(osic, module);
	c_module = generator_emit_module(osic, 0, l_exit);

	space_enclosing = osic->l_space_enclosing;
	osic->l_space_enclosing = node;

	for (stmt = node->u.module_stmt.stmt_list;
	     stmt;
	     stmt = stmt->sibling)
	{
		int n;
		struct syntax *name;
		struct syntax *accessor;
		struct syntax *accessor_list;

		if (!compiler_stmt(osic, stmt)) {
			return 0;
		}

		name = NULL;
		accessor_list = NULL;
		if (stmt->kind == SYNTAX_KIND_VAR_STMT) {
			name = stmt->u.var_stmt.name;
			accessor_list = stmt->u.var_stmt.accessor_list;
		}

		if (stmt->kind == SYNTAX_KIND_CLASS_STMT) {
			name = stmt->u.class_stmt.name;
			accessor_list = stmt->u.class_stmt.accessor_list;
		}

		if (stmt->kind == SYNTAX_KIND_DEFINE_STMT) {
			name = stmt->u.define_stmt.name;
			accessor_list = stmt->u.define_stmt.accessor_list;
		}

		stmt_enclosing = osic->l_stmt_enclosing;
		osic->l_stmt_enclosing = stmt;
		if (accessor_list) {
			n = 0;
			for (accessor = accessor_list;
			     accessor;
			     accessor = accessor->sibling)
			{
				if (node_is_getter(osic, accessor)) {
					if (!compiler_expr(osic, accessor)) {
						return 0;
					}
					n += 1;
				}
			}
			if (n) {
				if (!compiler_const_object(osic, module)) {
					return 0;
				}
				if (!compiler_const_string(osic,
				                           name->buffer))
				{
					return 0;
				}
				generator_emit_setgetter(osic, n);
			}

			n = 0;
			for (accessor = accessor_list;
			     accessor;
			     accessor = accessor->sibling)
			{
				if (node_is_setter(osic, accessor)) {
					if (!compiler_expr(osic, accessor)) {
						return 0;
					}
					n += 1;
				}
			}

			if (n) {
				if (!compiler_const_object(osic, module)) {
					return 0;
				}
				if (!compiler_const_string(osic,
				                           name->buffer))
				{
					return 0;
				}
				generator_emit_setsetter(osic, n);
			}
		}
		osic->l_stmt_enclosing = stmt_enclosing;
	}

	module_scope = osic->l_scope;
	for (symbol = module_scope->symbol; symbol; symbol = symbol->next) {
		oobject_set_item(osic,
		                 ((struct omodule *)module)->attr,
		                 ostring_create(osic,
		                                symbol->name,
		                                strlen(symbol->name)),
		                 ointeger_create_from_long(osic,
		                                           symbol->local));
	}

	generator_emit_label(osic, l_exit);
	generator_patch_module(osic, c_module, node->nlocals, l_exit);
	osic->l_space_enclosing = space_enclosing;

	return 1;
}

static int
compiler_import_stmt(struct osic *osic, struct syntax *node)
{
	struct generator_label *l_exit;
	struct generator_code *c_module;

	struct syntax *stmt;
	struct scope *scope;
	struct symbol *symbol;

	char *module_name;
	char *module_path;
	struct syntax *module_stmt;
	struct scope *module_scope;
	struct oobject *module;

	struct syntax *stmt_enclosing;
	struct syntax *space_enclosing;

	space_enclosing = osic->l_space_enclosing;
	module_path = node->u.import_stmt.path_string->buffer;
	module_path = resolve_module_path(osic, node, module_path);
	if (node->u.import_stmt.name) {
		module_name = node->u.import_stmt.name->buffer;
	} else {
		module_name = resolve_module_name(osic, module_path);
	}

	symbol = scope_add_symbol(osic,
	                          osic->l_scope,
	                          module_name,
	                          SYMBOL_LOCAL);
	if (!symbol) {
		compiler_error_redefined(osic, node, module_name);

		return 0;
	}
	symbol->local = space_enclosing->nlocals++;

	module = oobject_get_item(osic,
	                          osic->l_modules,
	                          ostring_create(osic,
	                                         module_path,
	                                         strlen(module_path)));
	if (module) {
		if (!compiler_const_object(osic, module)) {
			return 0;
		}
		generator_emit_store(osic, 0, symbol->local);

		return 1;
	}

#ifndef STATICLIB
	if (module_path_is_native(osic, module_path)) {
		typedef struct oobject *(*module_init_t)(struct osic *);
		void *handle;
		char *init_name;
		module_init_t module_init;

#ifdef WINDOWS
		handle = LoadLibrary(module_path);
		if (handle == NULL) {
			fprintf(stderr, "LoadLibrary: %s fail\n", module_path);

			return 0;
		}
#else
		handle = dlopen(module_path, RTLD_NOW);
		if (handle == NULL) {
			fprintf(stderr, "dlopen: %s\n", dlerror());

			return 0;
		}
#endif
		module_name = resolve_module_name(osic, module_path);

		init_name = arena_alloc(osic, osic->l_arena, OSIC_NAME_MAX);
		strcpy(init_name, module_name);
		strcat(init_name, "_module");
		/*
		 * cast 'void *' -> 'uintptr_t' -> function pointer
		 * can make compiler happy
		 */
#ifdef WINDOWS
		module_init = (module_init_t)GetProcAddress(handle, init_name);
#else
		module_init = (module_init_t)(uintptr_t)dlsym(handle,
		                                              init_name);
#endif
		if (!module_init) {
			return 0;
		}
		module = module_init(osic);
		if (!module) {
			return 0;
		}
		if (!compiler_const_object(osic, module)) {
			return 0;
		}
		generator_emit_store(osic, 0, symbol->local);

		oobject_set_item(osic,
	                         osic->l_modules,
		                 ostring_create(osic,
		                                module_path,
		                                strlen(module_path)),
		                 module);

		return 1;
	}
#endif

	if (!input_set_file(osic, module_path)) {
		char buffer[PATH_MAX];
		snprintf(buffer,
		         sizeof(buffer),
		         "open '%s' fail\n",
		         module_path);
		compiler_error(osic, node, buffer);

		return 0;
	}

	lexer_next_token(osic);
	module_stmt = parser_parse(osic);
	if (!module_stmt) {
		compiler_error(osic, node, "syntax error\n");

		return 0;
	}
	node->u.import_stmt.module_stmt = module_stmt;

	/* make module's scope start from NULL */
	scope = osic->l_scope;
	osic->l_scope = NULL;
	scope_enter(osic, SCOPE_MODULE);
	osic->l_space_enclosing = module_stmt;

	module = omodule_create(osic,
	                        ostring_create(osic,
	                                       module_name,
	                                       strlen(module_name)));

	if (!compiler_const_object(osic, module)) {
		return 0;
	}
	generator_emit_store(osic, 0, symbol->local);

	oobject_set_item(osic,
	                 osic->l_modules,
	                 ostring_create(osic,
	                                module_path,
	                                strlen(module_path)),
	                 module);
	if (!module) {
		return 0;
	}
	compiler_const_object(osic, module);

	l_exit = generator_make_label(osic);
	c_module = generator_emit_module(osic, 0, l_exit);
	for (stmt = module_stmt->u.module_stmt.stmt_list;
	     stmt;
	     stmt = stmt->sibling)
	{
		int n;
		struct syntax *name;
		struct syntax *accessor;
		struct syntax *accessor_list;

		if (!compiler_stmt(osic, stmt)) {
			return 0;
		}

		name = NULL;
		accessor_list = NULL;
		if (stmt->kind == SYNTAX_KIND_VAR_STMT) {
			name = stmt->u.var_stmt.name;
			accessor_list = stmt->u.var_stmt.accessor_list;
		}

		if (stmt->kind == SYNTAX_KIND_CLASS_STMT) {
			name = stmt->u.class_stmt.name;
			accessor_list = stmt->u.class_stmt.accessor_list;
		}

		if (stmt->kind == SYNTAX_KIND_DEFINE_STMT) {
			name = stmt->u.define_stmt.name;
			accessor_list = stmt->u.define_stmt.accessor_list;
		}

		stmt_enclosing = osic->l_stmt_enclosing;
		osic->l_stmt_enclosing = stmt;
		if (accessor_list) {
			n = 0;
			for (accessor = accessor_list;
			     accessor;
			     accessor = accessor->sibling)
			{
				if (node_is_getter(osic, accessor)) {
					if (!compiler_expr(osic, accessor)) {
						return 0;
					}
					n += 1;
				}
			}
			if (n) {
				if (!compiler_const_object(osic, module)) {
					return 0;
				}
				if (!compiler_const_string(osic,
				                           name->buffer))
				{
					return 0;
				}
				generator_emit_setgetter(osic, n);
			}

			n = 0;
			for (accessor = accessor_list;
			     accessor;
			     accessor = accessor->sibling)
			{
				if (node_is_setter(osic, accessor)) {
					if (!compiler_expr(osic, accessor)) {
						return 0;
					}
					n += 1;
				}
			}

			if (n) {
				if (!compiler_const_object(osic, module)) {
					return 0;
				}
				if (!compiler_const_string(osic,
				                           name->buffer))
				{
					return 0;
				}
				generator_emit_setsetter(osic, n);
			}
		}
		osic->l_stmt_enclosing = stmt_enclosing;
	}

	module_scope = osic->l_scope;
	for (symbol = module_scope->symbol; symbol; symbol = symbol->next) {
		oobject_set_item(osic,
		                 ((struct omodule *)module)->attr,
		                 ostring_create(osic,
		                                symbol->name,
		                                strlen(symbol->name)),
		                 ointeger_create_from_long(osic,
		                                           symbol->local));
	}

	if (!compiler_const_object(osic, osic->l_nil)) {
		return 0;
	}
	generator_emit_operative_code(osic, OPERATIVE_CODE_RETURN);

	generator_emit_label(osic, l_exit);
	generator_patch_module(osic, c_module, module_stmt->nlocals, l_exit);

	osic->l_space_enclosing = space_enclosing;
	scope_leave(osic);
	osic->l_scope = scope;

	return 1;
}

static int
compiler_expr(struct osic *osic, struct syntax *node)
{
	struct oobject *object;

	if (!node) {
		return 0;
	}

	switch (node->kind) {
	case SYNTAX_KIND_NIL:
		if (!compiler_nil(osic, node)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_TRUE:
		if (!compiler_true(osic, node)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_FALSE:
		if (!compiler_false(osic, node)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_SENTINEL:
		if (!compiler_sentinel(osic, node)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_SELF:
		generator_emit_operative_code(osic, OPERATIVE_CODE_SELF);
		break;

	case SYNTAX_KIND_SUPER:
		generator_emit_operative_code(osic, OPERATIVE_CODE_SUPER);
		break;

	case SYNTAX_KIND_NAME:
		if (!compiler_name(osic, node)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_NUMBER_LITERAL:
		if (node_is_integer(osic, node)) {
			object = ointeger_create_from_cstr(osic, node->buffer);
		} else {
			object = onumber_create_from_cstr(osic, node->buffer);
		}
		if (!compiler_const_object(osic, object)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_STRING_LITERAL:
		object = ostring_create(osic, node->buffer, node->length);
		if (!compiler_const_object(osic, object)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_ARRAY:
		if (!compiler_array(osic, node)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_DICTIONARY:
		if (!compiler_dictionary(osic, node)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_UNOP:
		if (!compiler_unop(osic, node)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_BINOP:
		compiler_binop(osic, node);
		break;

	case SYNTAX_KIND_CONDITIONAL:
		if (!compiler_conditional(osic, node)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_CALL:
		if (!compiler_call(osic, node)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_GET_ITEM:
		if (!compiler_expr(osic, node->u.get_item.left)) {
			return 0;
		}
		if (!compiler_expr(osic, node->u.get_item.right)) {
			return 0;
		}
		generator_emit_operative_code(osic, OPERATIVE_CODE_GETITEM);
		break;

	case SYNTAX_KIND_GET_ATTR:
		if (!compiler_expr(osic, node->u.get_attr.left)) {
			return 0;
		}
		if (!compiler_const_string(osic,
		                           node->u.get_attr.right->buffer))
		{
			return 0;
		}
		generator_emit_operative_code(osic, OPERATIVE_CODE_GETATTR);
		break;

	case SYNTAX_KIND_GET_SLICE:
		if (!compiler_expr(osic, node->u.get_slice.left)) {
			return 0;
		}

		if (node->u.get_slice.start) {
			if (!compiler_expr(osic, node->u.get_slice.start)) {
				return 0;
			}
		} else {
			object = ointeger_create_from_long(osic, 0);
			if (!compiler_const_object(osic, object)) {
				return 0;
			}
		}

		if (node->u.get_slice.stop) {
			if (!compiler_expr(osic, node->u.get_slice.stop)) {
				return 0;
			}
		} else {
			if (!compiler_const_object(osic, osic->l_nil)) {
				return 0;
			}
		}

		if (node->u.get_slice.step) {
			if (!compiler_expr(osic, node->u.get_slice.step)) {
				return 0;
			}
		} else {
			object = ointeger_create_from_long(osic, 1);
			if (!compiler_const_object(osic, object)) {
				return 0;
			}
		}
		generator_emit_operative_code(osic, OPERATIVE_CODE_GETSLICE);
		break;

	/* define is also a expr value */
	case SYNTAX_KIND_CLASS_STMT:
		if (!compiler_class_stmt(osic, node)) {
			return 0;
		}
			break;

	/* define is also a expr value */
	case SYNTAX_KIND_DEFINE_STMT:
		if (!compiler_define_stmt(osic, node)) {
			return 0;
		}
			break;

	case SYNTAX_KIND_ACCESSOR:
		if (!compiler_expr(osic, node->u.accessor.expr)) {
			return 0;
		}
		break;

	default:
		compiler_error(osic, node, "unknown syntax node\n");
		return 0;
	}

	return 1;
}

static int
compiler_stmt(struct osic *osic, struct syntax *node)
{
	if (!node) {
		return 0;
	}

	switch (node->kind) {
	case SYNTAX_KIND_ASSIGN_STMT:
		if (!compiler_assign_stmt(osic, node)) {
			return 0;
		}
	break;

	case SYNTAX_KIND_IF_STMT:
		if (!compiler_if_stmt(osic, node)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_FOR_STMT:
		if (!compiler_for_stmt(osic, node)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_FORIN_STMT:
		if (!compiler_forin_stmt(osic, node)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_WHILE_STMT:
		if (!compiler_while_stmt(osic, node)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_MODULE_STMT:
		if (!compiler_module(osic, node)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_BLOCK_STMT:
		scope_enter(osic, SCOPE_BLOCK);
		if (!compiler_block(osic, node)) {
			return 0;
		}
		scope_leave(osic);
		break;

	case SYNTAX_KIND_VAR_STMT:
		if (!compiler_var_stmt(osic, node)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_DEFINE_STMT:
		if (!compiler_define_stmt(osic, node)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_TRY_STMT:
		if (!compiler_try_stmt(osic, node)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_IMPORT_STMT:
		if (!compiler_import_stmt(osic, node)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_BREAK_STMT:
		if (!compiler_break_stmt(osic, node)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_CONTINUE_STMT:
		if (!compiler_continue_stmt(osic, node)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_DELETE_STMT:
		if (!compiler_delete_stmt(osic, node)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_RETURN_STMT:
		if (!compiler_return_stmt(osic, node)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_THROW_STMT:
		if (!compiler_throw_stmt(osic, node)) {
			return 0;
		}
		break;

	case SYNTAX_KIND_CLASS_STMT:
		if (!compiler_class_stmt(osic, node)) {
			return 0;
		}
		break;

	default:
		if (!compiler_expr(osic, node)) {
			return 0;
		}
	}
	return 1;
}

int
compiler_compile(struct osic *osic, struct syntax *node)
{
	return compiler_module(osic, node);
}
