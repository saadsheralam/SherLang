#include "mpc.h"

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

enum { SVAL_NUM, SVAL_ERR };
enum { ERR_DIV_ZERO, ERR_BAD_OP, ERR_BAD_NUM };

typedef struct {
  int type; 
  long num; 
  int err; 
} sval; 

sval sval_num(long x){
  sval v; 
  v.type = SVAL_NUM; 
  v.num = x; 
  return v; 
}

sval sval_err(int x){
  sval v; 
  v.type = SVAL_ERR; 
  v.err = x; 
  return v; 
}

void sval_print(sval v){
  switch(v.type){
    case SVAL_NUM: printf("%li\n", v.num); break; 
    case SVAL_ERR: 
      if (v.err == ERR_DIV_ZERO) {
        printf("Error: Division By Zero!");
      } 
      if (v.err == ERR_BAD_OP) {
        printf("Error: Invalid Operator!");
      }
      if (v.err == ERR_BAD_NUM) {
        printf("Error: Invalid Number!");
      }
    break; 
  }
}

// print sval with newline 
void sval_println(sval v) { sval_print(v); putchar('\n'); }


// basic count total nodes in tree
int number_of_nodes(mpc_ast_t* t){
  
  if(t->children_num == 0){
    return 1; 
  } 

  if(t->children_num >= 1){
    int total = 1;
    for(int i = 0; i < t->children_num; i++){
      total = total + number_of_nodes(t->children[i]); 
    }

    return total; 
  }

  return 0; 
}

sval eval_op(sval x, char* op, sval y) {

  // Error for invalid number
  if (x.type == SVAL_ERR) { return x; }
  if (y.type == SVAL_ERR) { return y; }

  // proceed with evaluation if valid number 
  if (strcmp(op, "+") == 0) { return sval_num(x.num + y.num); }
  if (strcmp(op, "-") == 0) { return sval_num(x.num - y.num); }
  if (strcmp(op, "*") == 0) { return sval_num(x.num * y.num); }
  if (strcmp(op, "/") == 0) { 

    // error if div by zero 
    if(y.num == 0){
      return sval_err(ERR_DIV_ZERO);
    }
    return sval_num(x.num / y.num); 
  }

  // error for invalid op 
  return sval_err(ERR_BAD_OP);
}

sval eval(mpc_ast_t* t){
  // If the node is tagged as a number 
  if(strstr(t->tag, "number")){
    errno = 0; 

    // convert num to long, error if conv not succesful
    long x = strtol(t->contents, NULL, 10); 

    if (errno == ERANGE) {
      return sval_err(ERR_BAD_NUM); 
    } 
    return sval_num(x); 
  }

  // the operator is always the second child, the first child is '('
  char* op = t->children[1]->contents; 

  sval x = eval(t->children[2]); 

  int i = 3; 
  while(strstr(t->children[i]->tag, "expr")){
    x = eval_op(x, op, eval(t->children[i]));
    i++;  
  }

  return x; 
}

int main(int argc, char** argv) {

  // create parsers   
  mpc_parser_t* Number   = mpc_new("number");
  mpc_parser_t* Operator = mpc_new("operator");
  mpc_parser_t* Expr     = mpc_new("expr");
  mpc_parser_t* SherLang    = mpc_new("lispy");
  
  // define polish notation grammar 
  mpca_lang(MPCA_LANG_DEFAULT,
    "                                                     \
      number   : /-?[0-9]+/ ;                             \
      operator : '+' | '-' | '*' | '/' ;                  \
      expr     : <number> | '(' <operator> <expr>+ ')' ;  \
      lispy    : /^/ <operator> <expr>+ /$/ ;             \
    ",
    Number, Operator, Expr, SherLang);
  
  puts("SherLang Version 0.0.0.0.2");
  puts("Press Ctrl+c to Exit\n");
  
  while (1) {
  
    char* input = readline("SherLang> ");
    add_history(input);
    
    mpc_result_t r;
    if (mpc_parse("<stdin>", input, SherLang, &r)) {
      // mpc_ast_print(r.output);
      sval result = eval(r.output); 
      sval_println(result); 
      mpc_ast_delete(r.output);
    } else {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }
    
    free(input);
  }
  
  // cleanup parsers
  mpc_cleanup(4, Number, Operator, Expr, SherLang);
  
  return 0;
}