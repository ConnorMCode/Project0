#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
  char *input = NULL;
  size_t len = 0;

  printf("wish> ");
  while (getline(&input, &len, stdin) != -1) {
    printf("You entered: %s", input);
    printf("wish> ");
  }

  free(input);
  return 0;
}  
