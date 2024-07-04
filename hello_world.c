#include <stdio.h>
#include <stdlib.h>

// if compiling on Windows
#ifdef _WIN32
#include <string.h>

static char buffer[2048];

// fake readline
char* readline(char* prompt) {
  fputs(prompt, stdout);
  fgets(buffer, 2048, stdin);
  char* cpy = malloc(strlen(buffer) + 1);
  strcpy(cpy, buffer);
  cpy[strlen(cpy) - 1] = '\0';
  return cpy;
}

// fake add_history
void add_history(char* unused) {}

// if compiling on MacOS, Linux
#else
#include <readline/readline.h>
#endif

int main(int argc, char** argv) {
  puts("SherLang Version 0.0.0.0.1");
  puts("Press Ctrl+C to Exit\n");

  while (1) {
    // get user input
    char* input = readline("SherLang> ");
    add_history(input);

    // echo input
    printf("%s\n", input);

    // free input
    free(input);
  }

  return 0;
}
