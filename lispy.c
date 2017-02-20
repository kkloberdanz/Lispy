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
#include "lispy.h"

#define VERSION          0.9
#define BUFF_SIZE       2048
#define NUM_PARSERS        6

mpc_parser_t* Number;
mpc_parser_t* Decimal;
mpc_parser_t* Sexpr;
mpc_parser_t* Symbol;
mpc_parser_t* Expr;
mpc_parser_t* Lispy;


/*
 * Constructors
 */

/* number type lval */
lval* lval_num(long x) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_NUM;
    v->num = x;
    return v;
}

/* double type (decimal type) */
lval* lval_dbl(double x) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_DBL;
    v->dbl = x;
    return v;
}

/* error type */
/*
lval* lval_err(int x) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_ERR;
    v->err = x;
    return v;
}
*/

/* error type */
lval* lval_err(char* m) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_ERR;
    v->err = malloc(strlen(m) + 1);
    strcpy(v->err, m);
    return v;
}

lval* lval_sym(char *s) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_SYM;
    v->sym = malloc(strlen(s) + 1);
    strcpy(v->sym, s);
    return v;
}

/* s-expression type */
lval* lval_sexpr(void) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

/*
 * Destructors
 */

int destroy_lval(lval* v) {
    free(v); 
    return 0;
}

/* Free memory */
int lval_del(lval *v) {
    switch (v->type) {
        case LVAL_NUM:
            break;

        case LVAL_ERR:
            free(v->err);
            break;

        case LVAL_SYM:
            free(v->sym);
            break;

        case LVAL_SEXPR:
            for (int i = 0; i < v->count; ++i) {
                lval_del(v->cell[i]);
            }
            free(v->cell);
            break;
    }
    free(v);
    return 0;
}

lval* lval_read_num(mpc_ast_t *t) {
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? lval_num(x) : lval_err("invalid number");
}

lval* lval_add(lval *v, lval *x) {
	v->count++;
	v->cell = realloc(v->cell, sizeof(lval*) * v->count);
	v->cell[v->count-1] = x;
	return v;
}

lval* lval_read(mpc_ast_t* t) {

  /* If Symbol or Number return conversion to that type */
  if (strstr(t->tag, "number")) { return lval_read_num(t); }
  if (strstr(t->tag, "symbol")) { return lval_sym(t->contents); }

  /* If root (>) or sexpr then create empty list */
  lval* x = NULL;
  if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); }
  if (strstr(t->tag, "sexpr"))  { x = lval_sexpr(); }

#ifdef DEBUG
  printf("%d\n", x);
  if (x == NULL) {
      printf("Warning: x is NULL\n");
  }
#endif
  /* Fill this list with any valid expression contained within */
  for (int i = 0; i < t->children_num; i++) {
    if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
    if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
    if (strcmp(t->children[i]->contents, "}") == 0) { continue; }
    if (strcmp(t->children[i]->contents, "{") == 0) { continue; }
    if (strcmp(t->children[i]->tag,  "regex") == 0) { continue; }
    x = lval_add(x, lval_read(t->children[i]));
  }

  return x;
}

void lval_expr_print(lval *v, char open, char close) {
  putchar(open);
  for (int i = 0; i < v->count; i++) {

    /* Print Value contained within */
    lval_print(v->cell[i]);

    /* Don't print trailing space if last element */
    if (i != (v->count-1)) {
      putchar(' ');
    }
  }
  putchar(close);
}

void lval_print(lval *v) {
	switch (v->type) {
		case LVAL_NUM:
			printf("%li", v->num);
			break;

		case LVAL_ERR:
			printf("Error: %s", v->err);
            break;

		case LVAL_SYM:
            printf("%s", v->sym);
            break;

		case LVAL_SEXPR:
            lval_expr_print(v, '(', ')');
            break;
	}
}

void lval_println(lval *v) {
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

/* NO LONGER USED */
lval* eval_op(lval* x, char* op, lval* y) {

    if (x->type == LVAL_ERR) { return x; }
    if (y->type == LVAL_ERR) { return y; }

    if (strcmp(op, "+") == 0) { 
        if ((y->type == LVAL_NUM) && (x->type == LVAL_NUM)) {
            return lval_num(x->num + y->num); 
        }

        if ((y->type == LVAL_DBL) && (x->type == LVAL_DBL)) {
            return lval_dbl(x->dbl + y->dbl); 
        }

        return lval_err("LERR_MISMATCHED_TYPES");
    }

    if (strcmp(op, "-") == 0) { 
        if ((y->type == LVAL_NUM) && (x->type == LVAL_NUM)) {
            return lval_num(x->num - y->num); 
        }

        if ((y->type == LVAL_DBL) && (x->type == LVAL_DBL)) {
            return lval_dbl(x->dbl - y->dbl); 
        }

        return lval_err("LERR_MISMATCHED_TYPES");
    }

    if (strcmp(op, "*") == 0) { 
        if ((y->type == LVAL_NUM) && (x->type == LVAL_NUM)) {
            return lval_num(x->num * y->num); 
        }

        if ((y->type == LVAL_DBL) && (x->type == LVAL_DBL)) {
            return lval_dbl(x->dbl * y->dbl); 
        }

        return lval_err("LERR_MISMATCHED_TYPES");
    }

    if (strcmp(op, "/") == 0) { 
        if ((y->type == LVAL_NUM) && (x->type == LVAL_NUM)) {
            return y->num == 0 ? lval_err("LERR_DIV_ZERO") : 
                                lval_num(x->num / y->num);
        }

        if ((y->type == LVAL_DBL) && (x->type == LVAL_DBL)) {
            return y->num == 0.0 ? lval_err("LERR_DIV_ZERO") : 
                                  lval_dbl(x->dbl / y->dbl);
        }

        return lval_err("LERR_MISMATCHED_TYPES");
    }

    if (strcmp(op, "%") == 0) { 
        if ((y->type == LVAL_NUM) && (x->type == LVAL_NUM)) {
            return y->num == 0 ? lval_err("LERR_DIV_ZERO") : 
                                 lval_num(x->num % y->num);
        }

        if ((y->type == LVAL_DBL) && (x->type == LVAL_DBL)) {
            return lval_err("OP_INCOMPAT_TYPE");
        }

        return lval_err("LERR_MISMATCHED_TYPES");
    }

    return lval_err("LERR_BAD_OP");
}

/* NO LONGER USED */
lval* eval(mpc_ast_t* t) {
    /* if tagged as number, return it directly */
    if (strstr(t->tag, "number")) {
        errno = 0;
        long x = strtol(t->contents, NULL, 10);
        return errno != ERANGE ? lval_num(x) : lval_err("LERR_BAD_NUM");
    }

    if (strstr(t->tag, "decimal")) {
        errno = 0;
        double x = strtod(t->contents, NULL);
        return errno != ERANGE ? lval_dbl(x) : lval_err("LERR_BAD_NUM");
    }

    /* the operator is always the second child */
    char* op = t->children[1]->contents;

    lval* x = eval(t->children[2]);
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

char* lval_type(lval *v) {
    switch (v->type) {
        case LVAL_NUM:
            return "LVAL_NUM";
            break;

        case LVAL_DBL:
            return "LVAL_DBL";
            break;

        case LVAL_ERR:
            return "LVAL_ERR";
            break;

        case LVAL_SYM:
            return "LVAL_SYM";
            break;

        case LVAL_SEXPR:
            return "LVAL_SEXPR";
            break;

        default:
            return "NONE_TYPE";
            break;
    }
}

int parse_and_interpret(char* input) { 
    mpc_result_t r;
    if (mpc_parse("<stdin>", input, Lispy, &r)) {

        lval* x = lval_eval(lval_read(r.output));
        lval_println(x);
        lval_del(x);

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
    return 0;
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

//######################
lval* lval_eval_sexpr(lval* v) {

  /* Evaluate Children */
  for (int i = 0; i < v->count; i++) {
    v->cell[i] = lval_eval(v->cell[i]);
  }

  /* Error Checking */
  for (int i = 0; i < v->count; i++) {
    if (v->cell[i]->type == LVAL_ERR) { return lval_take(v, i); }
  }

  /* Empty Expression */
  if (v->count == 0) { return v; }

  /* Single Expression */
  if (v->count == 1) { return lval_take(v, 0); }

  /* Ensure First Element is Symbol */
  lval* f = lval_pop(v, 0);
  if (f->type != LVAL_SYM) {
    lval_del(f); lval_del(v);
    return lval_err("S-expression Does not start with symbol!");
  }

  /* Call builtin with operator */
  lval* result = builtin_op(v, f->sym);
  lval_del(f);
  return result;
}

lval* lval_eval(lval* v) {
  /* Evaluate Sexpressions */
  if (v->type == LVAL_SEXPR) { return lval_eval_sexpr(v); }
  /* All other lval types remain the same */
  return v;
}

lval* lval_pop(lval* v, int i) {
  /* Find the item at "i" */
  lval* x = v->cell[i];

  /* Shift memory after the item at "i" over the top */
  memmove(&v->cell[i], &v->cell[i+1],
    sizeof(lval*) * (v->count-i-1));

  /* Decrease the count of items in the list */
  v->count--;

  /* Reallocate the memory used */
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  return x;
}

lval* lval_take(lval* v, int i) {
  lval* x = lval_pop(v, i);
  lval_del(v);
  return x;
}


lval* builtin_op(lval* a, char* op) {

  /* Ensure all arguments are numbers */
  for (int i = 0; i < a->count; i++) {
    if (a->cell[i]->type != LVAL_NUM) {
      lval_del(a);
      return lval_err("Cannot operate on non-number!");
    }
  }

  /* Pop the first element */
  lval* x = lval_pop(a, 0);

  /* If no arguments and sub then perform unary negation */
  if ((strcmp(op, "-") == 0) && a->count == 0) {
    x->num = -x->num;
  }

  /* While there are still elements remaining */
  while (a->count > 0) {

    /* Pop the next element */
    lval* y = lval_pop(a, 0);

    if (x->type != y->type) {
        x = lval_err("error: missmatched types\n");
        break;
    }

    if (strcmp(op, "+") == 0) { 
        switch (x->type) {
            case LVAL_NUM:
                x->num += y->num;
                break;

            case LVAL_DBL: 
                x->dbl += y->dbl;
                break;

            default:
                x = lval_err("error: '%s' is an invalid type for opperation '+'");
                break;

        }
    }
    if (strcmp(op, "-") == 0) { x->num -= y->num; }
    if (strcmp(op, "*") == 0) { x->num *= y->num; }
    if (strcmp(op, "/") == 0) {
      if (y->num == 0) {
        lval_del(x); lval_del(y);
        x = lval_err("Division By Zero!"); break;
      }
      x->num /= y->num;
    }
    if (strcmp(op, "%") == 0) {
      if (y->num == 0) {
        lval_del(x); lval_del(y);
        x = lval_err("Division By Zero!"); break;
      }
      x->num %= y->num;
    }

    lval_del(y);
  }

  lval_del(a);
  return x;
}

//######################

int run(int argc, char** argv) {
    /* Global parsers */
    Number   = mpc_new("number");
    Decimal  = mpc_new("decimal");
    Sexpr    = mpc_new("sexpr");
    Symbol   = mpc_new("symbol");
    Expr     = mpc_new("expr");
    Lispy    = mpc_new("lispy");

    /* Define them with the following Language */
    mpca_lang(MPCA_LANG_DEFAULT,
    "                                                       \
      number  : /-?[0-9]+/ ;                                \
      decimal : /-?[0-9]+\\.[0-9]+/ ;                       \
      sexpr   : '(' <expr>* ')' ;                           \
      symbol  : '+' | '-' | '*' | '/' | '%';                \
      expr    : <number> | <decimal> | <symbol> | <sexpr> ; \
      lispy   : /^/ <expr>* /$/ ;                           \
    ",
        Number, Decimal, Sexpr, Symbol, Expr, Lispy);

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
    mpc_cleanup(NUM_PARSERS, Number, Decimal, Sexpr, Symbol, Expr, Lispy);
    return 0;
}
