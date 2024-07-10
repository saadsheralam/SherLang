#ifndef SHERLANG_H
#define SHERLANG_H
#include "mpc.h"
#include<stdbool.h>


struct lval; 
struct lenv; 
typedef struct lval lval; 
typedef struct lenv lenv; 
typedef lval*(*lbuiltin)(lenv*, lval*); 


// Enum for LISP value types
enum { LVAL_ERR, LVAL_NUM, LVAL_BOOL, LVAL_SYM, LVAL_FUN, LVAL_SEXPR, LVAL_QEXPR }; 

struct lval {

  // Basic 
  int type;
  double num; 
  char* err;
  char* sym; 

  // Functions 
  lbuiltin builtin;
  lenv* env; 
  lval* formals; // function args 
  lval* body;  // function body

  // maintain count and pointer to list of sval* to represent s expressions/q expressions (this is the fundamental cons cell in LISP)
  int count;
  struct lval** cell;
};

// maintains mapping of variable names and LISP Values 
struct lenv{
  lenv* par; // parent environment to allow functions to access global environment (which contain other builtins)  
  int count; 
  char** syms; 
  lval** vals; 
}; 

// Function declarations
lval* lval_num(double x); 
lval* lval_err(char* fmt, ...); 
lval* lval_sym(char* s); 
lval* lval_sexpr(void);
lval* lval_qexpr(void); 
lval* lval_fun(lbuiltin func); 
lval* lval_lambda(lval* formals, lval* body); 
void lval_del(lval* v); 
lval* lval_add(lval* v, lval* x); 
lval* lval_pop(lval* v, int i); 
lval* lval_take(lval* v, int i); 
lval* lval_copy(lval* v); 
int lval_eq(lval* x, lval* y); 
lval* lval_read_num(mpc_ast_t* t); 
lval* lval_read(mpc_ast_t* t); 
lval* lval_eval_sexpr(lenv* e, lval* v); 
lval* lval_eval(lenv* e, lval* v); 
void lval_expr_print(lval* v, char open, char close); 
void lval_print(lval* v); 
void lval_println(lval* v); 
lval* lval_call(lenv* e, lval* f, lval* a); 
lenv* lenv_new(void); 
void lenv_del(lenv* v); 
lval* lenv_get(lenv* e, lval* k); 
void lenv_put(lenv* e, lval* k, lval* v); 
void lenv_def(lenv* e, lval* k, lval* v); 
lenv* lenv_copy(lenv* e); 
char* ltype_name(int t); 
lval* builtin_op(lenv* e, lval* a, char* op); 
lval* builtin_add(lenv* e, lval* a); 
lval* builtin_sub(lenv* e, lval* a); 
lval* builtin_mul(lenv* e, lval* a); 
lval* builtin_div(lenv* e, lval* a); 
lval* builtin_mod(lenv* e, lval* a); 
lval* builtin_ord(lenv* e, lval* a, char* op); 
lval* builtin_gt(lenv* e, lval* a); 
lval* builtin_lt(lenv* e, lval* a); 
lval* builtin_ge(lenv* e, lval* a); 
lval* builtin_le(lenv* e, lval* a); 
lval* builtin_or(lenv* e, lval* a); 
lval* builtin_and(lenv* e, lval* a); 
lval* builtin_not(lenv* e, lval* a); 
lval* builtin_cmp(lenv* e, lval* a, char* op); 
lval* builtin_head(lenv* e, lval* a); 
lval* builtin_tail(lenv* e, lval* a); 
lval* builtin_list(lenv* e, lval* a); 
lval* builtin_eval(lenv* e, lval* a); 
lval* builtin_join(lenv* e, lval* a); 
lval* lval_join(lval* x, lval* y); 
lval* builtin_len(lenv* e, lval* a); 
lval* builtin_cons(lenv* e, lval* a); 
lval* builtin_var(lenv* e, lval* a, char* func); 
lval* builtin_def(lenv* e, lval* a); 
lval* builtin_put(lenv* e, lval* a); 
lval* builtin_lambda(lenv* e, lval* a); 
lval* builtin_if(lenv* e, lval* a); 
void lenv_add_builtin(lenv* e, char* name, lbuiltin func); 
void lenv_add_builtins(lenv* e); 

#endif 
