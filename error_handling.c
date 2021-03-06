// okay, this is the error handling section

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
#include <editline/history.h>
#endif



//create enum types for possible lval types
enum { LVAL_NUM, LVAL_ERR};
enum {LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM};


typedef struct {
	int type;
	long num;
	int err;
} lval;

//create a new number type lval
lval lval_num(long x){
	lval v; //null initialisation
	v.type = LVAL_NUM;
	v.num = x;
	return v;
}

//create lval error type
lval lval_err(int x){
	lval v;
	v.type = LVAL_ERR;
	v.err = x;
	return v;
}

void lval_print(lval v){
	//needs to check if an error or not, to print correctly
	switch(v.type){
		case LVAL_NUM: printf("%li",v.num); break;

		//in case there is an error, check what type it is and print
		case LVAL_ERR:
			if(v.err == LERR_DIV_ZERO){
				printf("Error: Division by Zero!");
			}
			if(v.err == LERR_BAD_OP){
				printf("Error: Invalid Operator!");
			}
			if(v.err == LERR_BAD_NUM){
				printf("Error: Invalid Number!");
			}
			break;
	}
}
void lval_println(lval v){
	lval_print(v);
	putchar('\n');
}

// replace the eval op to deal with lvals as intended
//including with the errors
lval eval_op(lval x, char* op, lval y){
	if(x.type==LVAL_ERR){
		return x;
	}
	if(y.type==LVAL_ERR){
		return y;
	}

	//otherwise do the maths on the numbervalues
  if (strcmp(op, "+") == 0) { return lval_num(x.num + y.num); }
  if (strcmp(op, "-") == 0) { return lval_num(x.num - y.num); }
  if (strcmp(op, "*") == 0) { return lval_num(x.num * y.num); }
  if (strcmp(op, "/") == 0) {
    /* If second operand is zero return error */
    return y.num == 0
      ? lval_err(LERR_DIV_ZERO)
      : lval_num(x.num / y.num);
  }

  return lval_err(LERR_BAD_OP);
}

// the pointer now makes sense, since it's a pointer to an object
// since c is always pass by value, and not pass by reference
// you essentially have to force pass by reference in by using pointers
// and do it manually!
lval eval(mpc_ast_t* t){
	if(strstr(t->tag, "number")){
		//if there is an error in conversion
		// I don't understand what is happening here. ERANGE is not devined
		// and errno is always 0, so I don't know!
		errno = 0;
		long x = strtol(t->contents, NULL, 10);
		return errno!=ERANGE ? lval_num(x): lval_err(LERR_BAD_NUM);
	}
	//get the operation and the first operand!
	char* op = t->children[1]->contents;
	//evaluate the first operand to its maximum potential
	lval x = eval(t->children[2]);

	int i = 3;
	while(strstr(t->children[i]->tag, "expr")){
		x = eval_op(x, op, eval(t->children[i]));
		i++;
		//I'm not sure I'm understanding thie fully, the recursion. oh well
	}
	return x;
}

int main(int argc, char** argv) {
  
  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Operator = mpc_new("operator");
  mpc_parser_t* Expr = mpc_new("expr");
  mpc_parser_t* Lispy = mpc_new("lispy");
  
  mpca_lang(MPCA_LANG_DEFAULT,
    "                                                     \
      number   : /-?[0-9]+/ ;                             \
      operator : '+' | '-' | '*' | '/' ;                  \
      expr     : <number> | '(' <operator> <expr>+ ')' ;  \
      lispy    : /^/ <operator> <expr>+ /$/ ;             \
    ",
    Number, Operator, Expr, Lispy);
  
  puts("Lispy Version 0.0.0.0.3");
  puts("Press Ctrl+c to Exit\n");
  
  while (1) {
  
    char* input = readline("lispy> ");
    add_history(input);
    
    mpc_result_t r;
    if (mpc_parse("<stdin>", input, Lispy, &r)) {
      
      lval result = eval(r.output);
      lval_println(result);
      mpc_ast_delete(r.output);
      
    } else {    
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }
    
    free(input);
    
  }
  
  mpc_cleanup(4, Number, Operator, Expr, Lispy);
  
  return 0;
}
