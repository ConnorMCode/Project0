#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_PATH 10

char *path[MAX_PATH] = {"/bin"};

void printPath() {
  printf("Current path:\n");
  for (int i = 0; path[i] != NULL; i++){
    printf("%s\n", path[i]);
  }
}

int runCmd(char *cmd) {

  cmd[strcspn(cmd, "\n")] = 0;
  pid_t pid;
  int status;
  char cmdPath[256];
  
  for (int i = 0; path[i] != NULL; i++){
    strcpy(cmdPath, path[i]);
    if (cmdPath[strlen(cmdPath) - 1] != '/') {
      strcat(cmdPath, "/");
    }
    strcat(cmdPath, cmd);

    pid = fork();

    if (pid == 0){
      execl(cmdPath, cmd, (char *)NULL);
      perror("execl");
      exit(EXIT_FAILURE);
    } else if (pid < 0){
      perror("fork");
      return -1;
    } else {
      wait(NULL);
      return 0;
    }
  }
  return -1;
}

int main() {
  char *input = NULL;
  size_t len = 0;

  printf("wish> ");
  while (getline(&input, &len, stdin) != -1) {

    input[strcspn(input, "\n")] = 0;
    
    if (strcmp(input, "exit") == 0){
      printf("Exiting wish..\n");
      break;
    }
    if (strcmp(input, "path") == 0){
      printPath();
      printf("wish> ");
      continue;
    }
    runCmd(input);
    printf("wish> ");
  }

  free(input);
  return 0;
}  
