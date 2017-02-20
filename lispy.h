#ifndef LISPY_H
#define LISPY_H

/* lisp value */
typedef struct lval {
    int type;
    int count;
    union {
        long          num;
        double        dbl;
        char         *err;
        char         *sym;
        struct lval **cell;
    };
} lval;

/* lval types */
typedef enum {
    LVAL_NUM,
    LVAL_DBL,
    LVAL_ERR,
    LVAL_SYM,
    LVAL_SEXPR
} LVAL_TYPE;

/* error types */
typedef enum {
    LERR_DIV_ZERO,
    LERR_BAD_OP,
    LERR_BAD_NUM,
    LERR_MISMATCHED_TYPES,
    OP_INCOMPAT_TYPE
} LERR;

lval* lval_num(long x);
lval* lval_dbl(double x);
//lval* lval_err(int x);
lval* lval_err(char* m);
lval* lval_sym(char *s);
lval* lval_sexpr(void);
int destroy_lval(lval* v);
int lval_del(lval *v);
lval* lval_read_num(mpc_ast_t *t);
lval* lval_add(lval *v, lval *x);
lval* lval_read(mpc_ast_t *t);
void lval_expr_print(lval *v, char open, char close);
void lval_print(lval *v);
void lval_println(lval *v);
int number_of_nodes(mpc_ast_t* t);
lval* eval_op(lval* x, char* op, lval* y);
lval* eval(mpc_ast_t* t);
int display_greeting();
int parse_and_interpret(char* input);
int shell();
int fromfile(char* filename);
int run(int argc, char** argv);
lval* lval_eval(lval *v);
lval* lval_pop(lval *v, int i);
lval* lval_take(lval *v, int i);
lval* builtin_op(lval *a, char *op);

#endif /* LISPY_H */
