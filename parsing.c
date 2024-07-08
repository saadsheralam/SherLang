#include "mpc.h"
#include <math.h>

#ifdef _WIN32

static char buffer[2048];

char* readline(char* prompt) {
  fputs(prompt, stdout);
  fgets(buffer, 2048, stdin);
  char* cpy = malloc(strlen(buffer)+1);
  strcpy(cpy, buffer);
  cpy[strlen(cpy)-1] = '\0';
  return cpy;
}

void add_history(char* unused) {}

#else
#include <editline/readline.h>
#endif

/* Add SYM and SEXPR as possible lval types */
enum { LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_SEXPR, LVAL_QEXPR };

typedef struct lval {
  int type;
  double num; 

  // represent error and symbols as strings
  char* err;
  char* sym;

  // maintain count and pointer to list of sval* to represent s expressions (this is the fundamental cons cell in LISP)
  // here all s-expressions are stored recursively
  int count;
  struct lval** cell;
} lval;

// Function definitions
lval* lval_num(double x); 
lval* lval_err(char* m); 
lval* lval_sym(char* s); 
lval* lval_sexpr(void);
void lval_del(lval* v); 
lval* lval_add(lval* v, lval* x); 
void lval_expr_print(lval* v, char open, char close); 
void lval_print(lval* v); 
void lval_println(lval* v); 
lval* lval_read_num(mpc_ast_t* t);
lval* lval_read(mpc_ast_t* t);  
lval* lval_eval_sexpr(lval* v); 
lval* lval_eval(lval* v); 
lval* lval_pop(lval* v, int i); 
lval* lval_take(lval* v, int i); 
lval* builtin_op(lval* a, char* op); 
lval* builtin_head(lval* a); 
lval* builtin_tail(lval* a); 
lval* builtin_list(lval* a);
lval* builtin_eval(lval* a); 
lval* builtin_join(lval* a); 
lval* lval_join(lval* x, lval* y); 
lval* builtin(lval* a, char* func); 


// create pointer for lval num type 
lval* lval_num(double x) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_NUM;
  v->num = x;
  return v;
}

// create pointer for lval err type 
lval* lval_err(char* m) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_ERR;
  v->err = malloc(strlen(m) + 1);
  strcpy(v->err, m);
  return v;
}

// create pointer for lval symbol type 
lval* lval_sym(char* s) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SYM;
  v->sym = malloc(strlen(s) + 1);
  strcpy(v->sym, s);
  return v;
}

// create pointer for lval s-expression type 
lval* lval_sexpr(void) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

lval* lval_qexpr(void) {
  lval* v = malloc(sizeof(lval)); 
  v->type = LVAL_QEXPR; 
  v->count = 0; 
  v->cell = NULL; 
  return v; 
}

// cleanup memory allocated to lval 
void lval_del(lval* v) {

  switch (v->type) {
    case LVAL_NUM: break;    
    case LVAL_ERR: free(v->err); break;
    case LVAL_SYM: free(v->sym); break;

    case LVAL_QEXPR: 
    case LVAL_SEXPR:
      // delete all lvals within lval 
      for (int i = 0; i < v->count; i++) {
        lval_del(v->cell[i]);
      }
      free(v->cell);
    break;

  }
  
  free(v);
}

lval* lval_add(lval* v, lval* x) {
  v->count++;
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  v->cell[v->count-1] = x;
  return v;
}

void lval_expr_print(lval* v, char open, char close) {
  putchar(open);
  for (int i = 0; i < v->count; i++) {
    
    // print lval within cell 
    lval_print(v->cell[i]);
    
    // don't print trailing space 
    if (i != (v->count-1)) {
      putchar(' ');
    }
  }
  putchar(close);
}

void lval_print(lval* v) {
  switch (v->type) {
    case LVAL_NUM: printf("%.2f", v->num); break;
    case LVAL_ERR: printf("Error: %s", v->err); break;
    case LVAL_SYM: printf("%s", v->sym); break;
    case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
    case LVAL_QEXPR: lval_expr_print(v, '{', '}'); break; 
  }
}

void lval_println(lval* v) { 
  lval_print(v); 
  putchar('\n');   
}

lval* lval_read_num(mpc_ast_t* t) {
  errno = 0;
  double x = strtod(t->contents, NULL);
  if (errno == ERANGE) {
    return lval_err("invalid number");
  }
  
  // long x = strtol(t->contents, NULL, 10);
  // if(errno == ERANGE){
  //   return lval_err("invalid number"); 
  // } 


  return lval_num(x); 
}

lval* lval_read(mpc_ast_t* t) {

  // printf("Value: %s", t->contents); 
  
  if (strstr(t->tag, "number")) { 
    // printf("Number mai"); 
    return lval_read_num(t); 
  }

  if (strstr(t->tag, "symbol")) { 
    // printf("Symbol mai"); 
    return lval_sym(t->contents); 
  }
  
  // create empty list of lvals for root/s-expr
  lval* x = NULL;
  if (strcmp(t->tag, ">") == 0) { 
    x = lval_sexpr(); 
  } 

  if (strstr(t->tag, "sexpr"))  { 
    x = lval_sexpr(); 
  }

  if (strstr(t->tag, "qexpr"))  { 
    // printf("Start of qexpr"); 
    x = lval_qexpr(); 
  }

  
  // fill list with any valid expressions 
  for (int i = 0; i < t->children_num; i++) {
    if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
    if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
    if (strcmp(t->children[i]->contents, "{") == 0) { continue; }
    if (strcmp(t->children[i]->contents, "}") == 0) { continue; }
    if (strcmp(t->children[i]->tag,  "regex") == 0) { continue; }
    x = lval_add(x, lval_read(t->children[i]));
  }
  
  return x;
}

lval* lval_eval_sexpr(lval* v) {

  // evaluate children of s-expression 
  for (int i = 0; i < v->count; i++) {
    v->cell[i] = lval_eval(v->cell[i]);
  }

  // check if any children evaluates to an error
  for (int i = 0; i < v->count; i++) {
    if (v->cell[i]->type == LVAL_ERR) { 
      return lval_take(v, i); 
    }
  }

  // empty expression 
  if (v->count == 0) { 
    return v; 
  }

  // single expression 
  if (v->count == 1) { return lval_take(v, 0); }

  // ensure first element is symbol 
  lval* f = lval_pop(v, 0);
  if (f->type != LVAL_SYM) {
    lval_del(f); lval_del(v);
    return lval_err("S-expression Does not start with symbol!");
  }

  lval* result = builtin(v, f->sym);
  lval_del(f);
  return result;
}

lval* lval_eval(lval* v) {
  if (v->type == LVAL_SEXPR) { 
    return lval_eval_sexpr(v); 
  }
  return v;
}

lval* lval_pop(lval* v, int i) {
  lval* x = v->cell[i];

  // Shift memory after the item at "i" over the top 
  memmove(&v->cell[i], &v->cell[i+1], sizeof(lval*) * (v->count-i-1));
  v->count--;

  // Reallocate the memory used 
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  return x;
}

lval* lval_take(lval* v, int i) {
  lval* x = lval_pop(v, i);
  lval_del(v);
  return x;
}

lval* builtin_op(lval* a, char* op) {

  // Ensure all arguments are numbers 
  for (int i = 0; i < a->count; i++) {
    if (a->cell[i]->type != LVAL_NUM) {
      lval_del(a);
      return lval_err("Cannot operate on non-number!");
    }
  }

  lval* x = lval_pop(a, 0);

  // If no arguments and sub then perform unary negation 
  if ((strcmp(op, "-") == 0) && a->count == 0) {
    x->num = -x->num;
  }

  while (a->count > 0) {

    /* Pop the next element */
    lval* y = lval_pop(a, 0);

    if (strcmp(op, "+") == 0) { x->num += y->num; }
    if (strcmp(op, "-") == 0) { x->num -= y->num; }
    if (strcmp(op, "*") == 0) { x->num *= y->num; }
    if (strcmp(op, "%") == 0) { x->num = fmod(x->num, y->num); }
    if (strcmp(op, "/") == 0) {
      if (y->num == 0) {
        lval_del(x); lval_del(y);
        x = lval_err("Division By Zero!"); break;
      }
      x->num /= y->num;
    }

    lval_del(y);
  }

  lval_del(a); return x;
}

// define macro for error checking in builtin functions
#define LASSERT(args, cond, err) \
  if (!(cond)) { lval_del(args); return lval_err(err); }

lval* builtin_head(lval* a){
  // a should contain a valid q expr
  // q expr should not be empty 
  // head should only receive 1 argument  

  LASSERT(a, a->count == 1, "Function 'head' passed too many arguments!");
  LASSERT(a, a->cell[0]->type == LVAL_QEXPR, "Function 'head' passed incorrect type!");
  LASSERT(a, a->cell[0]->count != 0, "Function 'head' passed {}!");

  lval* v = lval_take(a, 0);
  while (v->count > 1) { lval_del(lval_pop(v, 1)); }
  return v;
}

lval* builtin_tail(lval* a){
  LASSERT(a, a->count == 1, "Function 'tail' passed too many arguments!");
  LASSERT(a, a->cell[0]->type == LVAL_QEXPR, "Function 'tail' passed incorrect type!");
  LASSERT(a, a->cell[0]->count != 0, "Function 'tail' passed {}!");

  lval* v = lval_take(a, 0);
  lval_del(lval_pop(v, 0));
  return v;
}

lval* builtin_list(lval* a) {
  a->type = LVAL_QEXPR;
  return a;
}

lval* builtin_eval(lval* a) {
  LASSERT(a, a->count == 1, "Function 'eval' passed too many arguments!");
  LASSERT(a, a->cell[0]->type == LVAL_QEXPR, "Function 'eval' passed incorrect type!");

  // change qexpr type to sexpr and evaluate
  lval* x = lval_take(a, 0);
  x->type = LVAL_SEXPR;
  return lval_eval(x);
}

lval* builtin_join(lval* a) {

  for (int i = 0; i < a->count; i++) {
    LASSERT(a, a->cell[i]->type == LVAL_QEXPR, "Function 'join' passed incorrect type.");
  }

  lval* x = lval_pop(a, 0);

  while (a->count) {
    x = lval_join(x, lval_pop(a, 0));
  }

  lval_del(a);
  return x;
}

lval* lval_join(lval* x, lval* y) {

  while (y->count) {
    x = lval_add(x, lval_pop(y, 0));
  }

  lval_del(y);
  return x;
}

lval* builtin_len(lval* a){
  LASSERT(a, a->count == 1, "Function 'len' passed too many arguments!");
  LASSERT(a, a->cell[0]->type == LVAL_QEXPR, "Function 'len' passed incorrect type!");

  double len = a->cell[0]->count; 
  lval_del(a); 
  return lval_num(len);    
}

lval* builtin_cons(lval* a){
  LASSERT(a, a->count == 2, "Function 'cons' passed too many/few arguments!");
  LASSERT(a, a->cell[0]->type == LVAL_NUM, "Function 'len' passed incorrect paramter, first argument should be a number!"); 
  LASSERT(a, a->cell[1]->type == LVAL_QEXPR, "Function 'len' passed incorrect paramter, second argument should be a q-expresion!"); 

  lval* v = lval_qexpr(); 
  lval_add(v, a->cell[0]); 
  v = lval_join(v, a->cell[1]); 
  return v;  
}


lval* builtin(lval* a, char* func) {
  if (strcmp("list", func) == 0) { return builtin_list(a); }
  if (strcmp("head", func) == 0) { return builtin_head(a); }
  if (strcmp("tail", func) == 0) { return builtin_tail(a); }
  if (strcmp("join", func) == 0) { return builtin_join(a); }
  if (strcmp("eval", func) == 0) { return builtin_eval(a); }
  if (strcmp("cons", func) == 0) { return builtin_cons(a); }
  if (strcmp("len", func) == 0) { return builtin_len(a); }
  if (strstr("+-/*", func)) { return builtin_op(a, func); }
  lval_del(a);
  return lval_err("Unknown Function!");
}

int main(int argc, char** argv) {
  
  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Symbol = mpc_new("symbol");
  mpc_parser_t* Sexpr  = mpc_new("sexpr");
  mpc_parser_t* Qexpr  = mpc_new("qexpr");
  mpc_parser_t* Expr   = mpc_new("expr");
  mpc_parser_t* SherLang  = mpc_new("sherlang");

  mpca_lang(MPCA_LANG_DEFAULT,
  "                                                                         \
    number : /[+-]?([0-9]*[.])?[0-9]+/ ;                                    \
    symbol : \"list\" | \"head\" | \"tail\"                                 \
           | \"join\" | \"eval\" | \"len\" | \"cons\" | '+' | '-' | '*' | '/' | '%';  \
    sexpr  : '(' <expr>* ')' ;                                              \
    qexpr  : '{' <expr>* '}' ;                                              \
    expr   : <number> | <symbol> | <sexpr> | <qexpr> ;                      \
    sherlang  : /^/ <expr>* /$/ ;                                           \
  ",
  Number, Symbol, Sexpr, Qexpr, Expr, SherLang);

  puts("SherLang Version 0.0.0.0.5");
  puts("Press Ctrl+c to Exit\n");
  
  while (1) {
  
    char* input = readline("SherLang> ");
    add_history(input);
    
    mpc_result_t r;
    if (mpc_parse("<stdin>", input, SherLang, &r)) {
      // lval* x = lval_eval(lval_read(r.output));
      // lval* x = lval_read(r.output);
      // printf("before");  
      // lval* x = lval_read(r.output); 
      // printf("after"); 
      lval* x = lval_eval(lval_read(r.output));
      lval_println(x);
      lval_del(x);
      mpc_ast_delete(r.output);
    } else {    
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }
    
    free(input);
    
  }
  
  mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, SherLang);
  
  return 0;
}