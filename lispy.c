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

mpc_parser_t* Number;
mpc_parser_t* Decimal;
mpc_parser_t* Operator;
mpc_parser_t* Expr;
mpc_parser_t* Lispy;

typedef struct {
    char* name;
    FILE* file;
} file_t;

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

/* double type (decimal type) */
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

/* display output to file */
void lval_print(FILE* file, FILE* error, lval v) {
    switch (v.type) {
        case LVAL_NUM:
            fprintf(file, "%li", v.num);
            break;

        case LVAL_ERR:
            switch (v.err) {
                case LERR_DIV_ZERO:
                    fprintf(error, "error: Division by Zero");
                    break;

                case LERR_BAD_OP:
                    fprintf(error, "error: Invalid Operator");
                    break;

                case LERR_BAD_NUM:
                    fprintf(error, "error: Invalid Number");
                    break;

                case LERR_MISMATCHED_TYPES:
                    fprintf(error, "error: Missmatched Types");
                    break; 

                case OP_INCOMPAT_TYPE:
                    fprintf(error, "error: Types incompatable with operator");
                    break; 
            }
            break;

        case LVAL_DBL:
            fprintf(file, "%lf", v.dbl);
            break;
    }
}

void lval_println(lval v) {
    lval_print(stdout, stderr, v);
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
        errno = 0;
        double x = strtod(t->contents, NULL);
        return errno != ERANGE ? lval_dbl(x) : lval_err(LERR_BAD_NUM);
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

int display_greeting() {
    printf("lispy version: %.2f\n", VERSION);
    printf("License: GPLv3 (see https://www.gnu.org/licenses/gpl-3.0.txt)\n");
#ifdef __GNUC__
    printf("GCC: %d.%d.%d\n", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#endif
    printf("CTRL-C to exit\n");

    return 0;
}

int parse_and_interpret(char* input) { 
    mpc_result_t r;
    if (mpc_parse("<stdin>", input, Lispy, &r)) {

        lval result = eval(r.output);
        lval_println(result);

#ifdef DEBUG
        /* On Success Print the AST */
        mpc_ast_print(r.output);
#endif
        mpc_ast_delete(r.output);
    } else {
        /* Otherwise Print the Error */
        mpc_err_print(r.error);
        mpc_err_delete(r.error);
    }
}

int shell() {
    int reti = display_greeting();
    char* input;
    while (1) {
        input = readline("[lispy]> ");
        add_history(input);
        if (strcmp("q", input) == 0) {
            break;
        }

        parse_and_interpret(input);

        free(input);
    }
    free(input);
    return 0;
}

int fromfile(char* filename) {
    printf("lispy: Reading from file %s\n", filename);
    return 0;
}

int run(int argc, char** argv) {
    /* Global parsers */
    Number   = mpc_new("number");
    Decimal  = mpc_new("decimal");
    Operator = mpc_new("operator");
    Expr     = mpc_new("expr");
    Lispy    = mpc_new("lispy");

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

    int reti;

    printf("argc = %d\n", argc);
    if (argc == 2) {
        reti = fromfile(argv[1]);
    } else {
        reti = shell();
    }

    return reti;
}

int main(int argc, char** argv) {
    int reti = run(argc, argv);

    /* Undefine and Delete our Parsers */
    mpc_cleanup(5, Number, Decimal, Operator, Expr, Lispy);
    return 0;
}
