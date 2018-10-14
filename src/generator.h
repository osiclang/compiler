#ifndef OSIC_GENERATOR_H
#define OSIC_GENERATOR_H

struct generator_label;

#define MAX_ARG 5

struct generator_arg {
	int type; /* 0: value, 1: label */
	int size; /* size of this value */
	int value;

	int address;
	struct generator_label *label;
	struct generator_arg *prevlabel;
};

struct generator_code {
	int operative_code;
	int address;
	int dead;
	struct generator_arg *arg[MAX_ARG];
	struct generator_label *label; /* if operative_code == -1 */

	struct generator_code *prev;
	struct generator_code *next;
};

struct generator_label {
	int count; /* reference count */
	int address;
	struct generator_code *code;
	struct generator_arg *prevlabel;
};

struct generator {
	int address;

	struct generator_code *head;
	struct generator_code *tail;
};

struct generator *
generator_create(struct osic *osic);

struct generator_code *
generator_make_code(struct osic *osic,
                    int operative_code,
                    struct generator_arg *arg1,
                    struct generator_arg *arg2,
                    struct generator_arg *arg3,
                    struct generator_arg *arg4,
                    struct generator_arg *arg5);

struct generator_arg *
generator_make_arg(struct osic *osic,
                   int type,
                   int size,
                   int value);

struct generator_arg *
generator_make_arg_label(struct osic *osic,
                         struct generator_label *label);

struct generator_label *
generator_make_label(struct osic *osic);

void
generator_emit_label(struct osic *osic,
                     struct generator_label *label);

struct generator_code *
generator_emit_code(struct osic *osic,
                    struct generator_code *code);

/* insert `newcode' before `code' */
void
generator_insert_code(struct osic *osic,
                      struct generator_code *code,
                      struct generator_code *newcode);

void
generator_delete_code(struct osic *osic,
                      struct generator_code *code);

struct generator_code *
generator_patch_code(struct osic *osic,
                     struct generator_code *oldcode,
                     struct generator_code *newcode);

void
generator_emit(struct osic *osic);

struct generator_code *
generator_emit_operative_code(struct osic *osic,
                      int operative_code);

struct generator_code *
generator_emit_const(struct osic *osic,
                     int pool);

struct generator_code *
generator_emit_unpack(struct osic *osic,
                      int count);

struct generator_code *
generator_emit_load(struct osic *osic,
                    int level,
                    int local);

struct generator_code *
generator_emit_store(struct osic *osic,
                     int level,
                     int local);

struct generator_code *
generator_emit_array(struct osic *osic,
                     int length);

struct generator_code *
generator_emit_dictionary(struct osic *osic,
                          int length);

struct generator_code *
generator_emit_jmp(struct osic *osic,
                   struct generator_label *label);

struct generator_code *
generator_emit_jz(struct osic *osic,
                  struct generator_label *label);

struct generator_code *
generator_emit_jnz(struct osic *osic,
                   struct generator_label *label);

struct generator_code *
generator_emit_call(struct osic *osic,
                    int argc);

struct generator_code *
generator_emit_tailcall(struct osic *osic,
                        int argc);

struct generator_code *
generator_emit_module(struct osic *osic,
                      int nlocals,
                      struct generator_label *label);

struct generator_code *
generator_patch_module(struct osic *osic,
                       struct generator_code *code,
                       int nlocals,
                       struct generator_label *label);

struct generator_code *
generator_emit_define(struct osic *osic,
                      int define,
                      int nvalues,
                      int nparams,
                      int nlocals,
                      struct generator_label *label);

struct generator_code *
generator_patch_define(struct osic *osic,
                       struct generator_code *code,
                       int define,
                       int nvalues,
                       int nparams,
                       int nlocals,
                       struct generator_label *label);

struct generator_code *
generator_emit_try(struct osic *osic,
                   struct generator_label *label);

struct generator_code *
generator_emit_class(struct osic *osic,
                     int nsupers,
                     int nattrs);

struct generator_code *
generator_emit_setgetter(struct osic *osic,
                         int ngetters);

struct generator_code *
generator_emit_setsetter(struct osic *osic,
                         int nsetters);

#endif /* osic_GENERATOR_H */
