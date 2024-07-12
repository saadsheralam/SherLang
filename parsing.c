#include "parsing.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>

#ifdef _WIN32

static char buffer[2048];

char *readline(char *prompt)
{
	fputs(prompt, stdout);
	fgets(buffer, 2048, stdin);
	char *cpy = malloc(strlen(buffer) + 1);
	strcpy(cpy, buffer);
	cpy[strlen(cpy) - 1] = '\0';
	return cpy;
}

void add_history(char *unused) {}

#else
#include <editline/readline.h>
#endif

/* LISP Value and Associated Functions */

// initalize lval num type
lval *lval_num(double x)
{
	lval *v = malloc(sizeof(lval));
	v->type = LVAL_NUM;
	v->num = x;
	return v;
}

// initialize lval err type
lval *lval_err(char *fmt, ...)
{
	lval *v = malloc(sizeof(lval));
	v->type = LVAL_ERR;

	va_list va;
	va_start(va, fmt);

	v->err = malloc(512);

	vsnprintf(v->err, 511, fmt, va);

	v->err = realloc(v->err, strlen(v->err) + 1);

	va_end(va);
	return v;
}

// initialize lval symbol type
lval *lval_sym(char *s)
{
	lval *v = malloc(sizeof(lval));
	v->type = LVAL_SYM;
	v->sym = malloc(strlen(s) + 1);
	strcpy(v->sym, s);
	return v;
}

// initialize lval s-expression type
lval *lval_sexpr(void)
{
	lval *v = malloc(sizeof(lval));
	v->type = LVAL_SEXPR;
	v->count = 0;
	v->cell = NULL;
	return v;
}

// initialize lval q-expression type
lval *lval_qexpr(void)
{
	lval *v = malloc(sizeof(lval));
	v->type = LVAL_QEXPR;
	v->count = 0;
	v->cell = NULL;
	return v;
}

// initialize lval func type (for bultin in func)
lval *lval_fun(lbuiltin func)
{
	lval *v = malloc(sizeof(lval));
	v->type = LVAL_FUN;
	v->builtin = func;
	return v;
}

// initialize lval lambda type (for user defined func)
lval *lval_lambda(lval *formals, lval *body)
{
	lval *v = malloc(sizeof(lval));
	v->type = LVAL_FUN;
	v->builtin = NULL; // this is used to differentiate between builtin and user defined functions
	v->env = lenv_new();
	v->formals = formals;
	v->body = body;
	return v;
}

// initialize bool type, by default is false
lval *lval_bool(void)
{
	lval *v = malloc(sizeof(lval));
	v->type = LVAL_BOOL;
	v->num = false;
	return v;
}

lval *lval_str(char *s)
{
	lval *v = malloc(sizeof(lval));
	v->type = LVAL_STR;
	v->str = malloc(strlen(s) + 1);
	strcpy(v->str, s);
	return v;
}

// cleanup memory allocated to lval
void lval_del(lval *v)
{

	switch (v->type)
	{
	case LVAL_NUM:
		break;
	case LVAL_BOOL:
		break;
	case LVAL_ERR:
		free(v->err);
		break;
	case LVAL_SYM:
		free(v->sym);
		break;
	case LVAL_STR:
		free(v->str);
		break;
	case LVAL_FUN:
		if (!v->builtin)
		{
			lenv_del(v->env);
			lval_del(v->formals);
			lval_del(v->body);
		}
		break;

	case LVAL_QEXPR:
	case LVAL_SEXPR:
		// delete all lvals within lval
		for (int i = 0; i < v->count; i++)
		{
			lval_del(v->cell[i]);
		}
		free(v->cell);
		break;
	}

	free(v);
}

// add value x to the expression in v
lval *lval_add(lval *v, lval *x)
{
	v->count++;
	v->cell = realloc(v->cell, sizeof(lval *) * v->count);
	v->cell[v->count - 1] = x;
	return v;
}

// pop value from LISP expression at index i
lval *lval_pop(lval *v, int i)
{
	lval *x = v->cell[i];

	// Shift memory after the item at "i" over the top
	memmove(&v->cell[i], &v->cell[i + 1], sizeof(lval *) * (v->count - i - 1));
	v->count--;

	// Reallocate the memory used
	v->cell = realloc(v->cell, sizeof(lval *) * v->count);

	return x;
}

// Pop value from expression at index i and delete the LISP Value
lval *lval_take(lval *v, int i)
{
	lval *x = lval_pop(v, i);
	lval_del(v);
	return x;
}

lval *lval_join(lval *x, lval *y)
{

	while (y->count)
	{
		x = lval_add(x, lval_pop(y, 0));
	}

	lval_del(y);
	return x;
}

// copy LISP value to a new LISP value. Does not delete previous value.
// Also copies the environments within LISP values.
lval *lval_copy(lval *v)
{
	lval *x = malloc(sizeof(lval));
	x->type = v->type;

	switch (v->type)
	{
	case LVAL_NUM:
		x->num = v->num;
		break;
	case LVAL_BOOL:
		x->num = v->num;
		break;
	case LVAL_FUN:
		if (v->builtin)
		{
			x->builtin = v->builtin;
		}
		else
		{
			x->builtin = NULL;
			x->env = lenv_copy(v->env);
			x->formals = lval_copy(v->formals);
			x->body = lval_copy(v->body);
		}
		break;

	case LVAL_ERR:
		x->err = malloc(strlen(v->err) + 1);
		strcpy(x->err, v->err);
		break;

	case LVAL_SYM:
		x->sym = malloc(strlen(v->sym) + 1);
		strcpy(x->sym, v->sym);
		break;

	case LVAL_STR:
		x->str = malloc(strlen(v->str) + 1);
		strcpy(x->str, v->str);
		break;

	case LVAL_SEXPR:
	case LVAL_QEXPR:
		x->count = v->count;
		x->cell = malloc(sizeof(lval *) * x->count);
		for (int i = 0; i < x->count; i++)
		{
			x->cell[i] = lval_copy(v->cell[i]);
		}
		break;
	}

	return x;
}

// check if two lval are equal
int lval_eq(lval *x, lval *y)
{

	if (x->type != y->type)
	{
		return 0;
	}

	// comparison based on type
	switch (x->type)
	{
	case LVAL_NUM:
		return (x->num == y->num);
	case LVAL_BOOL:
		return (x->num == y->num);

	case LVAL_ERR:
		return (strcmp(x->err, y->err) == 0);
	case LVAL_SYM:
		return (strcmp(x->sym, y->sym) == 0);
	case LVAL_STR:
		return (strcmp(x->str, y->str) == 0);

	case LVAL_FUN:
		if (x->builtin || y->builtin)
		{
			return x->builtin == y->builtin;
		}
		else
		{
			// for user defined functions, check both args and body
			return lval_eq(x->formals, y->formals) && lval_eq(x->body, y->body);
		}

	case LVAL_QEXPR:
	case LVAL_SEXPR:
		if (x->count != y->count)
		{
			return 0;
		}
		for (int i = 0; i < x->count; i++)
		{
			if (!lval_eq(x->cell[i], y->cell[i]))
			{
				return 0;
			}
		}
		return 1;
		break;
	}
	return 0;
}

// Reading LISP Values from terminal

lval *lval_read_str(char *s, int *i)
{

	char *lval_str_unescapable = "abfnrtv\\\'\"";
	// char *lval_str_escapable = "\a\b\f\n\r\t\v\\\'\"";

	char *part = calloc(1, 1);

	/* More forward one step past initial " character */
	(*i)++;
	while (s[*i] != '"')
	{

		char c = s[*i];

		/* If end of input then there is an unterminated string literal */
		if (c == '\0')
		{
			free(part);
			return lval_err("Unexpected end of input");
		}

		/* If backslash then unescape character after it */
		if (c == '\\')
		{
			(*i)++;
			/* Check next character is escapable */
			if (strchr(lval_str_unescapable, s[*i]))
			{
				c = lval_str_unescape(s[*i]);
			}
			else
			{
				free(part);
				return lval_err("Invalid escape sequence %c", s[*i]);
			}
		}

		part = realloc(part, strlen(part) + 2);
		part[strlen(part) + 1] = '\0';
		part[strlen(part) + 0] = c;
		(*i)++;
	}
	/* Move forward past final " character */
	(*i)++;

	lval *x = lval_str(part);

	free(part);
	return x;
}

lval *lval_read_sym(char *s, int *i)
{

	char *part = calloc(1, 1);

	/* While valid identifier characters */
	while (strchr(
			   "abcdefghijklmnopqrstuvwxyz"
			   "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			   "0123456789_.+-*\\/=<>!&",
			   s[*i]) &&
		   s[*i] != '\0')
	{

		part = realloc(part, strlen(part) + 2);
		part[strlen(part) + 1] = '\0';
		part[strlen(part) + 0] = s[*i];
		(*i)++;
	}

	int is_num = strchr("-.0123456789", part[0]) != NULL;
	for (int j = 1; j < strlen(part); j++)
	{
		if (strchr(".0123456789", part[j]) == NULL)
		{
			is_num = 0;
			break;
		}
	}
	if (strlen(part) == 1 && part[0] == '-')
	{
		is_num = 0;
	}

	lval *x = NULL;
	if (is_num)
	{
		errno = 0;
		double v = strtod(part, NULL);
		x = (errno != ERANGE) ? lval_num(v) : lval_err("Invalid Number %s", part);
	}
	else
	{
		x = lval_sym(part);
	}

	free(part);
	return x;
}

lval *lval_read_expr(char *s, int *i, char end)
{
	/* Either create new qexpr or sexpr */
	lval *x = (end == '}') ? lval_qexpr() : lval_sexpr();

	/* While not at end character keep reading lvals */
	while (s[*i] != end)
	{
		lval *y = lval_read(s, i);
		/* If an error then return this and stop */
		if (y->type == LVAL_ERR)
		{
			lval_del(x);
			return y;
		}
		else
		{
			lval_add(x, y);
		}
	}

	/* Move past end character */
	(*i)++;
	return x;
}

lval *lval_read(char *s, int *i)
{
	/* Skip all trailing whitespace and comments */
	while (strchr(" \t\v\r\n;", s[*i]) && s[*i] != '\0')
	{
		if (s[*i] == ';')
		{
			while (s[*i] != '\n' && s[*i] != '\0')
			{
				(*i)++;
			}
		}
		(*i)++;
	}

	lval *x = NULL;

	/* If we reach end of input then we're missing something */
	if (s[*i] == '\0')
	{
		return lval_err("Unexpected end of input");
	}

	/* If next character is ( then read S-Expr */
	else if (s[*i] == '(')
	{
		(*i)++;
		x = lval_read_expr(s, i, ')');
	}

	/* If next character is { then read Q-Expr */
	else if (s[*i] == '{')
	{
		(*i)++;
		x = lval_read_expr(s, i, '}');
	}

	/* If next character is part of a symbol then read symbol */
	else if (strchr(
				 "abcdefghijklmnopqrstuvwxyz"
				 "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
				 "0123456789_+-*\\/=<>!&",
				 s[*i]))
	{
		x = lval_read_sym(s, i);
	}

	/* If next character is " then read string */
	else if (strchr("\"", s[*i]))
	{
		x = lval_read_str(s, i);
	}

	/* Encountered some unexpected character */
	else
	{
		x = lval_err("Unexpected character %c", s[*i]);
	}

	/* Skip all trailing whitespace and comments */
	while (strchr(" \t\v\r\n;", s[*i]) && s[*i] != '\0')
	{
		if (s[*i] == ';')
		{
			while (s[*i] != '\n' && s[*i] != '\0')
			{
				(*i)++;
			}
		}
		(*i)++;
	}

	return x;
}

// Evaluating expressions in LISP values
lval *lval_eval_sexpr(lenv *e, lval *v)
{

	// evaluate children of s-expression
	for (int i = 0; i < v->count; i++)
	{
		v->cell[i] = lval_eval(e, v->cell[i]);
	}

	// check if any children evaluates to an error
	for (int i = 0; i < v->count; i++)
	{
		if (v->cell[i]->type == LVAL_ERR)
		{
			return lval_take(v, i);
		}
	}

	// empty expression
	if (v->count == 0)
	{
		return v;
	}

	// single expression
	if (v->count == 1)
	{
		return lval_take(v, 0);
	}

	// ensure first element is function
	lval *f = lval_pop(v, 0);
	if (f->type != LVAL_FUN)
	{
		lval *err = lval_err("S-Expression starts with incorrect type. Got %s, Expected %s.", ltype_name(f->type), ltype_name(LVAL_FUN));
		lval_del(v);
		lval_del(f);
		return err;
	}

	lval *result = lval_call(e, f, v);
	lval_del(f);
	return result;
}

lval *lval_eval(lenv *e, lval *v)
{
	if (v->type == LVAL_SYM)
	{
		lval *x = lenv_get(e, v);
		lval_del(v);
		return x;
	}

	if (v->type == LVAL_SEXPR)
	{
		return lval_eval_sexpr(e, v);
	}
	return v;
}

// Printing LISP values to terminal
void lval_expr_print(lval *v, char open, char close)
{
	if (v->count == 0)
	{
		printf("ok");
	}
	else
	{
		putchar(open);
		for (int i = 0; i < v->count; i++)
		{

			// print lval within cell
			lval_print(v->cell[i]);

			// don't print trailing space
			if (i != (v->count - 1))
			{
				putchar(' ');
			}
		}
		putchar(close);
	}
}

void lval_print_str(lval *v)
{
	char *lval_str_escapable = "\a\b\f\n\r\t\v\\\'\"";
	putchar('"');
	/* Loop over the characters in the string */
	for (int i = 0; i < strlen(v->str); i++)
	{
		if (strchr(lval_str_escapable, v->str[i]))
		{
			/* If the character is escapable then escape it */
			printf("%s", lval_str_escape(v->str[i]));
		}
		else
		{
			/* Otherwise print character as it is */
			putchar(v->str[i]);
		}
	}
	putchar('"');
}

void lval_print(lval *v)
{
	switch (v->type)
	{
	case LVAL_NUM:
		printf("%.2f", v->num);
		break;
	case LVAL_BOOL:
		printf("%s", v->num ? "true" : "false");
		break;
	case LVAL_ERR:
		printf("Error: %s", v->err);
		break;
	case LVAL_SYM:
		printf("%s", v->sym);
		break;
	case LVAL_STR:
		lval_print_str(v);
		break;
	case LVAL_SEXPR:
		lval_expr_print(v, '(', ')');
		break;
	case LVAL_QEXPR:
		lval_expr_print(v, '{', '}');
		break;
	case LVAL_FUN:
		if (v->builtin)
		{
			printf("<builtin>");
		}
		else
		{
			printf("(\\ ");
			lval_print(v->formals);
			putchar(' ');
			lval_print(v->body);
			putchar(')');
		}
		break;
	}
}

void lval_println(lval *v)
{
	lval_print(v);
	putchar('\n');
}

lval *lval_call(lenv *e, lval *f, lval *a)
{

	if (f->builtin)
	{
		return f->builtin(e, a);
	}

	int given = a->count;
	int total = f->formals->count;

	while (a->count)
	{

		if (f->formals->count == 0)
		{
			lval_del(a);
			return lval_err("Function passed too many arguments. Got %i, Expected %i.", given, total);
		}

		lval *sym = lval_pop(f->formals, 0);

		if (strcmp(sym->sym, "&") == 0)
		{
			if (f->formals->count != 1)
			{
				lval_del(a);
				return lval_err("Function format invalid. Symbol '&' not followed by single symbol.");
			}

			lval *nsym = lval_pop(f->formals, 0);
			lenv_put(f->env, nsym, builtin_list(e, a));
			lval_del(sym);
			lval_del(nsym);
			break;
		}

		lval *val = lval_pop(a, 0);

		lenv_put(f->env, sym, val);
		lval_del(sym);
		lval_del(val);
	}

	lval_del(a);

	if (f->formals->count > 0 && strcmp(f->formals->cell[0]->sym, "&") == 0)
	{

		// Check to ensure that & is not passed invalidly.
		if (f->formals->count != 2)
		{
			return lval_err("Function format invalid. Symbol '&' not followed by single symbol.");
		}

		// Pop and delete '&' symbol
		lval_del(lval_pop(f->formals, 0));

		// Pop next symbol and create empty list
		lval *sym = lval_pop(f->formals, 0);
		lval *val = lval_qexpr();

		// Bind to environment and delete
		lenv_put(f->env, sym, val);
		lval_del(sym);
		lval_del(val);
	}

	if (f->formals->count == 0)
	{
		f->env->par = e;
		return builtin_eval(f->env, lval_add(lval_sexpr(), lval_copy(f->body)));
	}
	else
	{
		return lval_copy(f);
	}
}

/* LISP Environment and Associated Functions */

// initalize new env
lenv *lenv_new(void)
{
	lenv *v = malloc(sizeof(lenv));
	v->par = NULL;
	v->count = 0;
	v->syms = NULL;
	v->vals = NULL;
	return v;
}

// delete env and cleanup memory
void lenv_del(lenv *v)
{
	for (int i = 0; i < v->count; i++)
	{
		free(v->syms[i]);
		lval_del(v->vals[i]);
	}

	free(v->syms);
	free(v->vals);
	free(v);
}

// get value of variable k (lval* k) from environment (lenv* e)
// also recursively check in parent environment for variable.
lval *lenv_get(lenv *e, lval *k)
{
	for (int i = 0; i < e->count; i++)
	{
		if (strcmp(e->syms[i], k->sym) == 0)
		{
			return lval_copy(e->vals[i]);
		}
	}

	// check also in parent env
	if (e->par)
	{
		return lenv_get(e->par, k);
	}
	else
	{
		return lval_err("Unbound Symbol '%s'", k->sym);
	}
}

// Put a variable and its value in the environment
// Puts in the local enviroment (notice no check for parent env)
void lenv_put(lenv *e, lval *k, lval *v)
{

	// check if variable already exists in env
	for (int i = 0; i < e->count; i++)
	{
		if (strcmp(e->syms[i], k->sym) == 0)
		{
			lval_del(e->vals[i]); // delete current value
			e->vals[i] = lval_copy(v);
			return;
		}
	}

	// if variable does not exist, allocate space and add new variable to env
	e->count++;
	e->vals = realloc(e->vals, sizeof(lval *) * e->count);
	e->syms = realloc(e->syms, sizeof(lval *) * e->count);

	// copy contents
	e->vals[e->count - 1] = lval_copy(v);
	e->syms[e->count - 1] = malloc(strlen(k->sym) + 1);
	strcpy(e->syms[e->count - 1], k->sym);
}

// define a varialbe in a global environment
void lenv_def(lenv *e, lval *k, lval *v)
{
	while (e->par)
	{
		e = e->par;
	}
	lenv_put(e, k, v);
}

// copy environment to a new LIST environment
// does not delete previous environmnet
lenv *lenv_copy(lenv *e)
{
	lenv *n = malloc(sizeof(lenv));
	n->par = e->par;
	n->count = e->count;
	n->syms = malloc(sizeof(char *) * n->count);
	n->vals = malloc(sizeof(char *) * n->count);
	for (int i = 0; i < n->count; i++)
	{
		n->syms[i] = malloc(strlen(e->syms[i]) + 1);
		strcpy(n->syms[i], e->syms[i]);
		n->vals[i] = lval_copy(e->vals[i]);
	}

	return n;
}

/* Builtin Functions*/

// define macros and helper functions for error checking in builtin functions
#define LASSERT(args, cond, fmt, ...)             \
	if (!(cond))                                  \
	{                                             \
		lval *err = lval_err(fmt, ##__VA_ARGS__); \
		lval_del(args);                           \
		return err;                               \
	}

#define LASSERT_TYPE(func, args, index, expect)                     \
	LASSERT(args, args->cell[index]->type == expect,                \
			"Function '%s' passed incorrect type for argument %i. " \
			"Got %s, Expected %s.",                                 \
			func, index, ltype_name(args->cell[index]->type), ltype_name(expect))

#define LASSERT_NUM(func, args, num)                               \
	LASSERT(args, args->count == num,                              \
			"Function '%s' passed incorrect number of arguments. " \
			"Got %i, Expected %i.",                                \
			func, args->count, num)

#define LASSERT_NOT_EMPTY(func, args, index)                                       \
	LASSERT(args, args->cell[index]->count != 0 || args->cell[index]->str != NULL, \
			"Function '%s' passed {} for argument %i.", func, index);

#define LASSERT_TWOTYPES(func, args, index, type1, type2)                                      \
	LASSERT(args, args->cell[index]->type == type1 || args->cell[index]->type == type2,        \
			"Function '%s' passed incorrect type for argument %i. Got %s, exepected %s or %s", \
			func, index, ltype_name(args->cell[index]->type), ltype_name(type1), ltype_name(type2))

char *ltype_name(int t)
{
	switch (t)
	{
	case LVAL_FUN:
		return "Function";
	case LVAL_NUM:
		return "Number";
	case LVAL_BOOL:
		return "Boolean";
	case LVAL_ERR:
		return "Error";
	case LVAL_SYM:
		return "Symbol";
	case LVAL_STR:
		return "String";
	case LVAL_SEXPR:
		return "S-Expression";
	case LVAL_QEXPR:
		return "Q-Expression";
	default:
		return "Unknown";
	}
}

lval *builtin_op(lenv *e, lval *a, char *op)
{

	// Ensure all arguments are numbers
	for (int i = 0; i < a->count; i++)
	{
		if (a->cell[i]->type != LVAL_NUM)
		{
			char *type = ltype_name(a->cell[i]->type);
			lval_del(a);
			return lval_err("Function '%s' passed incorrect type for argument %i. Got %s, Expected Number.", op, i, type);
		}
	}

	lval *x = lval_pop(a, 0);

	// If no arguments and sub then perform unary negation
	if ((strcmp(op, "-") == 0) && a->count == 0)
	{
		x->num = -x->num;
	}

	while (a->count > 0)
	{

		/* Pop the next element */
		lval *y = lval_pop(a, 0);

		if (strcmp(op, "+") == 0)
		{
			x->num += y->num;
		}
		if (strcmp(op, "-") == 0)
		{
			x->num -= y->num;
		}
		if (strcmp(op, "*") == 0)
		{
			x->num *= y->num;
		}
		if (strcmp(op, "%") == 0)
		{
			x->num = fmod(x->num, y->num);
		}
		if (strcmp(op, "/") == 0)
		{
			if (y->num == 0)
			{
				lval_del(x);
				lval_del(y);
				x = lval_err("Division By Zero!");
				break;
			}
			x->num /= y->num;
		}

		lval_del(y);
	}

	lval_del(a);
	return x;
}

lval *builtin_add(lenv *e, lval *a)
{
	return builtin_op(e, a, "+");
}

lval *builtin_sub(lenv *e, lval *a)
{
	return builtin_op(e, a, "-");
}

lval *builtin_mul(lenv *e, lval *a)
{
	return builtin_op(e, a, "*");
}

lval *builtin_div(lenv *e, lval *a)
{
	return builtin_op(e, a, "/");
}

lval *builtin_mod(lenv *e, lval *a)
{
	return builtin_op(e, a, "%");
}

lval *builtin_ord(lenv *e, lval *a, char *op)
{
	bool r;

	if (strcmp(op, "!") == 0)
	{
		LASSERT_NUM(op, a, 1);

		if (a->cell[0]->type != LVAL_NUM && a->cell[0]->type != LVAL_BOOL)
		{
			lval_err("Function '%s' passed incorrect type for argument 0. Got %s, Expected %s.", a->cell[0]->type, "Boolean or Number");
		}

		r = !(a->cell[0]->num);
	}
	else
	{

		LASSERT_NUM(op, a, 2);
		// LASSERT_TYPE(op, a, 0, LVAL_NUM);
		// LASSERT_TYPE(op, a, 1, LVAL_NUM);

		// check types, could be num or bool. both args need not be of the same type
		if (a->cell[0]->type != LVAL_NUM && a->cell[0]->type != LVAL_BOOL)
		{
			lval_err("Function '%s' passed incorrect type for argument 0. Got %s, Expected %s.", a->cell[0]->type, "Boolean or Number");
		}

		if (a->cell[1]->type != LVAL_NUM && a->cell[1]->type != LVAL_BOOL)
		{
			lval_err("Function '%s' passed incorrect type for argument 1. Got %s, Expected %s.", a->cell[1]->type, "Boolean or Number");
		}

		if (strcmp(op, ">") == 0)
		{
			r = (a->cell[0]->num > a->cell[1]->num);
		}
		if (strcmp(op, "<") == 0)
		{
			r = (a->cell[0]->num < a->cell[1]->num);
		}
		if (strcmp(op, ">=") == 0)
		{
			r = (a->cell[0]->num >= a->cell[1]->num);
		}
		if (strcmp(op, "<=") == 0)
		{
			r = (a->cell[0]->num <= a->cell[1]->num);
		}
		if (strcmp(op, "||") == 0)
		{
			r = (a->cell[0]->num || a->cell[1]->num);
		}
		if (strcmp(op, "&&") == 0)
		{
			r = (a->cell[0]->num && a->cell[1]->num);
		}
	}

	lval_del(a);
	lval *b = lval_bool();
	b->num = r;
	return b;
}

lval *builtin_gt(lenv *e, lval *a)
{
	return builtin_ord(e, a, ">");
}

lval *builtin_lt(lenv *e, lval *a)
{
	return builtin_ord(e, a, "<");
}

lval *builtin_ge(lenv *e, lval *a)
{
	return builtin_ord(e, a, ">=");
}

lval *builtin_le(lenv *e, lval *a)
{
	return builtin_ord(e, a, "<=");
}

lval *builtin_or(lenv *e, lval *a)
{
	return builtin_ord(e, a, "||");
}

lval *builtin_and(lenv *e, lval *a)
{
	return builtin_ord(e, a, "&&");
}

lval *builtin_not(lenv *e, lval *a)
{
	return builtin_ord(e, a, "!");
}

lval *builtin_cmp(lenv *e, lval *a, char *op)
{
	LASSERT_NUM(op, a, 2);
	bool r;
	if (strcmp(op, "==") == 0)
	{
		r = lval_eq(a->cell[0], a->cell[1]);
	}
	if (strcmp(op, "!=") == 0)
	{
		r = !lval_eq(a->cell[0], a->cell[1]);
	}
	lval_del(a);
	lval *b = lval_bool();
	b->num = r;
	return b;
	return b;
}

lval *builtin_eq(lenv *e, lval *a)
{
	return builtin_cmp(e, a, "==");
}

lval *builtin_ne(lenv *e, lval *a)
{
	return builtin_cmp(e, a, "!=");
}

lval *builtin_head(lenv *e, lval *a)
{
	// a should contain a valid q expr
	// q expr should not be empty
	// head should only receive 1 argument
	LASSERT_NUM("head", a, 1);
	LASSERT_TWOTYPES("head", a, 0, LVAL_QEXPR, LVAL_STR);
	LASSERT_NOT_EMPTY("head", a, 1);

	lval *v = lval_take(a, 0); // takes the first arg
	if (v->type == LVAL_QEXPR)
	{
		while (v->count > 1)
		{
			// lval* x = lval_pop(v,1);
			// lval_print(x);
			// printf("\n");
			lval_del(lval_pop(v, 1));
		}
	}

	if (v->type == LVAL_STR)
	{
		memset((v->str) + 1, 0, strlen(v->str) * sizeof(char));
		char *first = realloc(v->str, sizeof(char));
		v = lval_str(first);
	}

	return v;
}

lval *builtin_tail(lenv *e, lval *a)
{
	LASSERT_NUM("tail", a, 1);
	LASSERT_TWOTYPES("tail", a, 0, LVAL_QEXPR, LVAL_STR);
	LASSERT_NOT_EMPTY("tail", a, 1);

	lval *v = lval_take(a, 0);

	if (v->type == LVAL_QEXPR)
	{
		lval_del(lval_pop(v, 0));
	}

	if (v->type == LVAL_STR)
	{
		v = lval_str(v->str + 1);
	}

	return v;
}

lval *builtin_list(lenv *e, lval *a)
{
	a->type = LVAL_QEXPR;
	return a;
}

lval *builtin_eval(lenv *e, lval *a)
{
	LASSERT_NUM("eval", a, 1);
	LASSERT_TYPE("eval", a, 0, LVAL_QEXPR);

	// change qexpr type to sexpr and evaluate
	lval *x = lval_take(a, 0);
	x->type = LVAL_SEXPR;
	return lval_eval(e, x);
}

lval *builtin_join(lenv *e, lval *a)
{

	bool not_all_qexpr = false;
	bool not_all_strs = false;

	for (int i = 0; i < a->count; i++)
	{
		if (a->cell[i]->type != LVAL_QEXPR)
		{
			not_all_qexpr = true;
		}
		// LASSERT(a, a->cell[i]->type == LVAL_QEXPR, "Function 'join' passed incorrect type. Got %s, Expected %s.", ltype_name(a->cell[0]->type), ltype_name(LVAL_QEXPR));
	}

	for (int i = 0; i < a->count; i++)
	{
		if (a->cell[i]->type != LVAL_STR)
		{
			not_all_strs = true;
		}
	}

	// either all should be qexprs or all should be strs
	if (not_all_qexpr ^ not_all_strs)
	{
		lval *x;

		if (not_all_strs)
		{
			x = lval_pop(a, 0);

			while (a->count)
			{
				x = lval_join(x, lval_pop(a, 0));
			}

			lval_del(a);
		}

		if (not_all_qexpr)
		{

			// allocated memory for larger string and concatenate
			int total_size = 0;
			for (int i = 0; i < a->count; i++)
			{
				total_size += strlen(a->cell[i]->str);
			}

			// allocate mem for a larger string
			char *concat_str = malloc(sizeof(char) * (total_size + 1));

			// concatenate strings
			for (int i = 0; i < a->count; i++)
			{
				strcat(concat_str, a->cell[i]->str);
			}

			lval_del(a);
			x = lval_str(concat_str);
		}

		return x;
	}
	else
	{

		if (not_all_qexpr)
		{
			for (int i = 0; i < a->count; i++)
			{
				LASSERT(a, a->cell[i]->type == LVAL_QEXPR, "Function 'join' passed incorrect type for argument %i. Got %s, Expected %s.", i, ltype_name(a->cell[i]->type), ltype_name(LVAL_QEXPR));
			}
		}

		if (not_all_strs)
		{
			for (int i = 0; i < a->count; i++)
			{
				LASSERT(a, a->cell[i]->type == LVAL_STR, "Function 'join' passed incorrect type for argument %i. Got %s, Expected %s.", i, ltype_name(a->cell[i]->type), ltype_name(LVAL_STR));
			}
		}

		return lval_err("Incorrect types to join. Only Q-expressions or strings can be joined.");
	}
}

lval *builtin_len(lenv *e, lval *a)
{
	LASSERT_NUM("len", a, 1);
	LASSERT_TYPE("len", a, 0, LVAL_QEXPR);

	double len = a->cell[0]->count;
	lval_del(a);
	return lval_num(len);
}

lval *builtin_cons(lenv *e, lval *a)
{
	LASSERT_NUM("cons", a, 2);
	LASSERT_TYPE("cons", a, 0, LVAL_NUM);
	LASSERT_TYPE("cons", a, 1, LVAL_QEXPR);

	lval *v = lval_qexpr();
	lval_add(v, a->cell[0]);
	v = lval_join(v, a->cell[1]);
	return v;
}

lval *builtin_var(lenv *e, lval *a, char *func)
{
	LASSERT_TYPE(func, a, 0, LVAL_QEXPR);

	lval *syms = a->cell[0];
	for (int i = 0; i < syms->count; i++)
	{
		LASSERT(a, (syms->cell[i]->type == LVAL_SYM), "Function '%s' cannot define non-symbol. Got %s, Expected %s.", func, ltype_name(syms->cell[i]->type), ltype_name(LVAL_SYM));
	}

	LASSERT(a, (syms->count == a->count - 1), "Function '%s' passed too many arguments for symbols. Got %i, Expected %i.", func, syms->count, a->count - 1);

	for (int i = 0; i < syms->count; i++)
	{
		/* If 'def' define in globally. If 'put' define in locally */
		if (strcmp(func, "def") == 0)
		{
			lenv_def(e, syms->cell[i], a->cell[i + 1]);
		}

		if (strcmp(func, "=") == 0)
		{
			lenv_put(e, syms->cell[i], a->cell[i + 1]);
		}
	}

	lval_del(a);
	return lval_sexpr();
}

lval *builtin_def(lenv *e, lval *a)
{
	// LASSERT(a, a->cell[0]->type == LVAL_QEXPR, "Function 'def' passed incorrect type. Got %s, Expected %s", ltype_name(a->cell[0]->type), ltype_name(LVAL_QEXPR));

	// // first argument is symbol list
	// lval* syms = a->cell[0];

	// for(int i = 0; i < syms->count; i++){
	//   LASSERT(a, syms->cell[i]->type == LVAL_SYM, "Function 'def' can not define non-symbol. Got %s", ltype_name(syms->cell[i]->type));
	// }

	// LASSERT(a, syms->count == a->count - 1, "Function 'def' can not define incorrect number of values to symbols. Got %i symbols and %i values", syms->count, a->count-1);

	// // assign values to symbols
	// for(int i = 0; i < syms->count; i++){
	//   lenv_put(e, syms->cell[i], a->cell[i+1]);
	// }

	// lval_del(a);
	// return lval_sexpr();

	return builtin_var(e, a, "def");
}

lval *builtin_put(lenv *e, lval *a)
{
	return builtin_var(e, a, "=");
}

lval *builtin_lambda(lenv *e, lval *a)
{
	LASSERT_NUM("\\", a, 2);
	LASSERT_TYPE("\\", a, 0, LVAL_QEXPR);
	LASSERT_TYPE("\\", a, 1, LVAL_QEXPR);

	// first q expression should only contain symbols
	for (int i = 0; i < a->cell[0]->count; i++)
	{
		LASSERT(a, (a->cell[0]->cell[i]->type) == LVAL_SYM, "Cannot define non-symbol. Got %s, Expected %s.", ltype_name(a->cell[0]->cell[i]->type), ltype_name(LVAL_SYM))
	}

	lval *formals = lval_pop(a, 0);
	lval *body = lval_pop(a, 0);
	lval_del(a);

	return lval_lambda(formals, body);
}

lval *builtin_if(lenv *e, lval *a)
{
	LASSERT_NUM("if", a, 3);
	LASSERT_TWOTYPES("if", a, 0, LVAL_NUM, LVAL_BOOL);
	LASSERT_TYPE("if", a, 1, LVAL_QEXPR);
	LASSERT_TYPE("if", a, 2, LVAL_QEXPR);

	// to evaluate both expressions
	a->cell[1]->type = LVAL_SEXPR;
	a->cell[2]->type = LVAL_SEXPR;

	lval *x;
	if (a->cell[0]->num)
	{
		x = lval_eval(e, lval_pop(a, 1));
	}
	else
	{
		x = lval_eval(e, lval_pop(a, 2));
	}

	lval_del(a);
	return x;
}

// load contents from a file given file name as a string
lval *builtin_load(lenv *e, lval *a)
{
	LASSERT_NUM("load", a, 1);
	LASSERT_TYPE("load", a, 0, LVAL_STR);

	FILE *f = fopen(a->cell[0]->str, "rb");
	if (f == NULL)
	{
		lval *err = lval_err("Could not load Library %s", a->cell[0]->str);
		lval_del(a);
		return err;
	}

	// Read File Contents
	fseek(f, 0, SEEK_END);
	long length = ftell(f);
	fseek(f, 0, SEEK_SET);
	char *input = calloc(length + 1, 1);
	fread(input, 1, length, f);
	fclose(f);

	// Read from input to create an S-Expr
	int pos = 0;
	lval *expr = lval_read_expr(input, &pos, '\0');
	free(input);

	// Evaluate all expressions contained in S-Expr
	if (expr->type != LVAL_ERR)
	{
		while (expr->count)
		{
			lval *x = lval_eval(e, lval_pop(expr, 0));
			if (x->type == LVAL_ERR)
			{
				lval_println(x);
			}
			lval_del(x);
		}
	}
	else
	{
		lval_println(expr);
	}

	lval_del(expr);
	lval_del(a);

	return lval_sexpr();
}

lval *builtin_print(lenv *e, lval *a)
{

	/* Print each argument followed by a space */
	for (int i = 0; i < a->count; i++)
	{
		lval_print(a->cell[i]);
		putchar(' ');
	}

	/* Print a newline and delete arguments */
	putchar('\n');
	lval_del(a);

	return lval_sexpr();
}

lval *builtin_error(lenv *e, lval *a)
{
	LASSERT_NUM("error", a, 1);
	LASSERT_TYPE("error", a, 0, LVAL_STR);

	/* Construct Error from first argument */
	lval *err = lval_err(a->cell[0]->str);

	/* Delete arguments and return */
	lval_del(a);
	return err;
}

// add builtin functions to the environment
void lenv_add_builtin(lenv *e, char *name, lbuiltin func)
{
	lval *k = lval_sym(name);
	lval *v = lval_fun(func);
	lenv_put(e, k, v);
	lval_del(k);
	lval_del(v);
}

void lenv_add_builtins(lenv *e)
{
	/* List Functions */
	lenv_add_builtin(e, "list", builtin_list);
	lenv_add_builtin(e, "head", builtin_head);
	lenv_add_builtin(e, "tail", builtin_tail);
	lenv_add_builtin(e, "eval", builtin_eval);
	lenv_add_builtin(e, "join", builtin_join);
	lenv_add_builtin(e, "len", builtin_len);
	lenv_add_builtin(e, "cons", builtin_cons);
	lenv_add_builtin(e, "\\", builtin_lambda);
	lenv_add_builtin(e, "def", builtin_def);
	lenv_add_builtin(e, "=", builtin_put);
	lenv_add_builtin(e, "if", builtin_if);

	/* Mathematical Functions */
	lenv_add_builtin(e, "+", builtin_add);
	lenv_add_builtin(e, "-", builtin_sub);
	lenv_add_builtin(e, "*", builtin_mul);
	lenv_add_builtin(e, "/", builtin_div);
	lenv_add_builtin(e, "%", builtin_mod);

	/* Comparison Functions */
	lenv_add_builtin(e, "if", builtin_if);
	lenv_add_builtin(e, "==", builtin_eq);
	lenv_add_builtin(e, "!=", builtin_ne);
	lenv_add_builtin(e, ">", builtin_gt);
	lenv_add_builtin(e, "<", builtin_lt);
	lenv_add_builtin(e, ">=", builtin_ge);
	lenv_add_builtin(e, "<=", builtin_le);
	lenv_add_builtin(e, "||", builtin_or);
	lenv_add_builtin(e, "&&", builtin_and);
	lenv_add_builtin(e, "!", builtin_not);

	/* String Functions */
	lenv_add_builtin(e, "load", builtin_load);
	lenv_add_builtin(e, "error", builtin_error);
	lenv_add_builtin(e, "print", builtin_print);
}

// lval* builtin(lval* a, char* func) {
//   if (strcmp("list", func) == 0) { return builtin_list(a); }
//   if (strcmp("head", func) == 0) { return builtin_head(a); }
//   if (strcmp("tail", func) == 0) { return builtin_tail(a); }
//   if (strcmp("join", func) == 0) { return builtin_join(a); }
//   if (strcmp("eval", func) == 0) { return builtin_eval(a); }
//   if (strcmp("cons", func) == 0) { return builtin_cons(a); }
//   if (strcmp("len", func) == 0) { return builtin_len(a); }
//   if (strstr("+-/*", func)) { return builtin_op(a, func); }
//   lval_del(a);
//   return lval_err("Unknown Function!");
// }

char lval_str_unescape(char x)
{
	switch (x)
	{
	case 'a':
		return '\a';
	case 'b':
		return '\b';
	case 'f':
		return '\f';
	case 'n':
		return '\n';
	case 'r':
		return '\r';
	case 't':
		return '\t';
	case 'v':
		return '\v';
	case '\\':
		return '\\';
	case '\'':
		return '\'';
	case '\"':
		return '\"';
	}
	return '\0';
}

char *lval_str_escape(char x)
{
	switch (x)
	{
	case '\a':
		return "\\a";
	case '\b':
		return "\\b";
	case '\f':
		return "\\f";
	case '\n':
		return "\\n";
	case '\r':
		return "\\r";
	case '\t':
		return "\\t";
	case '\v':
		return "\\v";
	case '\\':
		return "\\\\";
	case '\'':
		return "\\\'";
	case '\"':
		return "\\\"";
	}
	return "";
}

int main(int argc, char **argv)
{

	lenv *e = lenv_new();
	lenv_add_builtins(e);

	// load standard library
	lval *std_name = lval_str("stdlib.slang");
	lval *load_args = lval_qexpr();
	lval_add(load_args, std_name);
	builtin_load(e, load_args);

	//   printf("Current Environment: \n");
	//   for(int i = 0; i < e->count; i++){
	//     printf("%s", e->syms[i]);
	//     printf(" ");
	//     lval_print(e->vals[i]);
	//     printf("\n");
	//   }

	//   printf("\n");

	// Run interactive prompt in terminal
	if (argc == 1)
	{
		puts("SherLang Version 0.0.0.0.5");
		puts("Press Ctrl+c to Exit\n");

		while (1)
		{
			char *input = readline("SherLang> ");
			add_history(input);

			int pos = 0;
			lval *expr = lval_read_expr(input, &pos, '\0');

			lval *x = lval_eval(e, expr);
			lval_println(x);
			lval_del(x);
			free(input);
		}
	}

	// execute code from a file
	if (argc >= 2)
	{
		// for each file name supplied
		for (int i = 1; i < argc; i++)
		{

			lval *args = lval_add(lval_sexpr(), lval_str(argv[i]));
			lval *x = builtin_load(e, args);
			if (x->type == LVAL_ERR)
			{
				lval_println(x);
			}

			lval_del(x);
		}
	}

	lenv_del(e);
	return 0;
}