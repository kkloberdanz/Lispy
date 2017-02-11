/*
 * Programmer: Kyle Kloberdanz
 * Date Created: 10 Feb 2016
 * License: GNU GPLv3 (see LICENSE.txt)
 * From: http://buildyourownlisp.com/
 */

#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>
#include <editline/history.h>

#include "mpc.h"

#define VERSION   0.8
#define BUFF_SIZE 2048

/* lisp value */
typedef struct {
    int type;
    union {
        long   num;
        int    err;
        double dbl;
    };
} lval;

/* lval types */
typedef enum {
    LVAL_NUM,
    LVAL_ERR,
    LVAL_DBL
} LVAL_TYPE;

/* error types */
typedef enum {
    LERR_DIV_ZERO,
    LERR_BAD_OP,
    LERR_BAD_NUM,
    LERR_MISMATCHED_TYPES,
    OP_INCOMPAT_TYPE
} LERR;

/* number type lval */
lval lval_num(long x) {
    lval v;
    v.type = LVAL_NUM;
    v.num = x;
    return v;
}

lval lval_dbl(double x) {
    lval v;
    v.type = LVAL_DBL;
    v.dbl = x;
    return v;
}

/* error type */
lval lval_err(int x) {
    lval v;
    v.type = LVAL_ERR;
    v.err = x;
    return v;
}

void lval_print(lval v) {
    switch (v.type) {
        case LVAL_NUM:
            printf("%li", v.num);
            break;

        case LVAL_ERR:
            switch (v.err) {
                case LERR_DIV_ZERO:
                    printf("error: Division by Zero");
                    break;

                case LERR_BAD_OP:
                    printf("error: Invalid Operator");
                    break;

                case LERR_BAD_NUM:
                    printf("error: Invalid Number");
                    break;

                case LERR_MISMATCHED_TYPES:
                    printf("error: Missmatched Types");
                    break; 

                case OP_INCOMPAT_TYPE:
                    printf("error: Types incompatable with operator");
                    break; 
            }
            break;

        case LVAL_DBL:
            printf("%lf", v.dbl);
            break;
    }
}

void lval_println(lval v) {
    lval_print(v);
    putchar('\n');
}

int number_of_nodes(mpc_ast_t* t) {
    if (t->children_num == 0) {
        return 1;
    }

    if (t->children_num >= 1) {
        int total = 1;
        for (int i = 0; i < t->children_num; ++i) {
            total += number_of_nodes(t->children[i]);
        }
        return total;
    }
    return 0;
}

/* TODO: extend to include double operations */
lval eval_op(lval x, char* op, lval y) {

    if (x.type == LVAL_ERR) { return x; }
    if (y.type == LVAL_ERR) { return y; }

    if (strcmp(op, "+") == 0) { 
        if ((y.type == LVAL_NUM) && (x.type == LVAL_NUM)) {
            return lval_num(x.num + y.num); 
        }

        if ((y.type == LVAL_DBL) && (x.type == LVAL_DBL)) {
            return lval_dbl(x.dbl + y.dbl); 
        }

        return lval_err(LERR_MISMATCHED_TYPES);
    }

    if (strcmp(op, "-") == 0) { 
        if ((y.type == LVAL_NUM) && (x.type == LVAL_NUM)) {
            return lval_num(x.num - y.num); 
        }

        if ((y.type == LVAL_DBL) && (x.type == LVAL_DBL)) {
            return lval_dbl(x.dbl - y.dbl); 
        }

        return lval_err(LERR_MISMATCHED_TYPES);
    }

    if (strcmp(op, "*") == 0) { 
        if ((y.type == LVAL_NUM) && (x.type == LVAL_NUM)) {
            return lval_num(x.num * y.num); 
        }

        if ((y.type == LVAL_DBL) && (x.type == LVAL_DBL)) {
            return lval_dbl(x.dbl * y.dbl); 
        }

        return lval_err(LERR_MISMATCHED_TYPES);
    }

    if (strcmp(op, "/") == 0) { 
        if ((y.type == LVAL_NUM) && (x.type == LVAL_NUM)) {
            return y.num == 0 ? lval_err(LERR_DIV_ZERO) : 
                                lval_num(x.num / y.num);
        }

        if ((y.type == LVAL_DBL) && (x.type == LVAL_DBL)) {
            return y.num == 0.0 ? lval_err(LERR_DIV_ZERO) : 
                                  lval_dbl(x.dbl / y.dbl);
        }

        return lval_err(LERR_MISMATCHED_TYPES);
    }

    if (strcmp(op, "%") == 0) { 
        if ((y.type == LVAL_NUM) && (x.type == LVAL_NUM)) {
            return y.num == 0 ? lval_err(LERR_DIV_ZERO) : 
                                lval_num(x.num % y.num);
        }

        if ((y.type == LVAL_DBL) && (x.type == LVAL_DBL)) {
            return lval_err(OP_INCOMPAT_TYPE);
        }

        return lval_err(LERR_MISMATCHED_TYPES);
    }

    return lval_err(LERR_BAD_OP);
}

lval eval(mpc_ast_t* t) {
    /* if tagged as number, return it directly */
    if (strstr(t->tag, "number")) {
        errno = 0;
        long x = strtol(t->contents, NULL, 10);
        return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
    }

    if (strstr(t->tag, "decimal")) {
        double x = atof(t->contents);
        return lval_dbl(x);
    }

    /* the operator is always the second child */
    char* op = t->children[1]->contents;

    lval x = eval(t->children[2]);
    int i = 3;
    while (strstr(t->children[i]->tag, "expr")) {
        x = eval_op(x, op, eval(t->children[i]));
        i++;
    }

    return x;
}

int main(int argc, char** argv) {

	/* Create Some Parsers */
	mpc_parser_t* Number   = mpc_new("number");
	mpc_parser_t* Decimal  = mpc_new("decimal");
	mpc_parser_t* Operator = mpc_new("operator");
	mpc_parser_t* Expr     = mpc_new("expr");
	mpc_parser_t* Lispy    = mpc_new("lispy");

	/* Define them with the following Language */
	mpca_lang(MPCA_LANG_DEFAULT,
	  "                                                     \
		number   : /-?[0-9]+/ ;                             \
        decimal  : /-?[0-9]+\\.[0-9]+/ ;                    \
		operator : '+' | '-' | '*' | '/' | '%' ;            \
		expr     : <decimal> | <number> | '(' <operator> <expr>+ ')' ;  \
		lispy    : /^/ <operator> <expr>+ /$/ ;             \
	  ",
		Number, Decimal, Operator, Expr, Lispy);

    printf("lispy version: %.2f\n", VERSION);
    printf("License: GPLv3 (see https://www.gnu.org/licenses/gpl-3.0.txt)\n");
#ifdef __GNUC__
    printf("GCC: %d.%d.%d\n", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#endif
    printf("CTRL-C to exit\n");

    while (1) {
        char* input = readline("[lispy]> ");
        add_history(input);

        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Lispy, &r)) {

            lval result = eval(r.output);
            lval_println(result);
            mpc_ast_delete(r.output);

#ifdef DEBUG
            /* On Success Print the AST */
            mpc_ast_print(r.output);
            mpc_ast_delete(r.output);
#endif
        } else {
            /* Otherwise Print the Error */
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }
        free(input);
    }
    return 0;

    /* Undefine and Delete our Parsers */
    mpc_cleanup(5, Number, Decimal, Operator, Expr, Lispy);
    return 0;
}
