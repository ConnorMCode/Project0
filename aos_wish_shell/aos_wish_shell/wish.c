#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

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
  char *args[256];
  int argCount = 0;
  char *saveptr = NULL;

  
  char *redirection = strstr(cmd, ">");
  char *outputFile = NULL;

  if (redirection != NULL) {
    *redirection = '\0';
    redirection++;
    outputFile = strtok_r(redirection, " ", &saveptr);

    if (outputFile == NULL || strtok_r(NULL, " ", &saveptr) != NULL){
      char error_message[30] = "An error has occured\n";
      write(STDERR_FILENO, error_message, strlen(error_message));
      return -1;
    }
  }
  
  char *temp = strtok_r(cmd, " ", &saveptr);
  while (temp != NULL) {
    args[argCount++] = temp;
    temp = strtok_r(NULL, " ", &saveptr);
  }
  args[argCount] = NULL;
  
  for (int i = 0; path[i] != NULL; i++){
    strcpy(cmdPath, path[i]);
    if (cmdPath[strlen(cmdPath) - 1] != '/') {
      strcat(cmdPath, "/");
    }
    strcat(cmdPath, args[0]);

    pid = fork();

    if (pid == 0){

      if (outputFile != NULL){
	int out = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC);
	if (out == -1){
	  char error_message[30] = "An error has occurred\n";
          write(STDERR_FILENO, error_message, strlen(error_message));
          exit(EXIT_FAILURE);
	}

	dup2(out, STDOUT_FILENO);
	dup2(out, STDERR_FILENO);
	close(out);
      }
      
      execv(cmdPath, args);
      char error_message[30] = "An error has occurred\n"; 
      write(STDERR_FILENO, error_message, strlen(error_message));
      exit(EXIT_FAILURE);
    } else if (pid < 0){
      char error_message[30] = "An error has occurred\n"; 
      write(STDERR_FILENO, error_message, strlen(error_message));
      return -1;
    } else {
      wait(NULL);
      return 0;
    }
  }
  char error_message[30] = "An error has occurred\n"; 
  write(STDERR_FILENO, error_message, strlen(error_message));
  return -1;
}

void processLines(char *prompt, FILE *src) {
  char *input = NULL;
  size_t len = 0;

  printf("%s", prompt);
  while (getline(&input, &len, src) != -1) {

    input[strcspn(input, "\n")] = 0;
    
    if (strcmp(input, "exit") == 0){
      printf("Exiting wish..\n");
      break;
    }
    if (strcmp(input, "path") == 0){
      printPath();
      printf("%s", prompt);
      continue;
    }

    if (strncmp(input, "cd ", 3) == 0) {
      char *arg = strtok(input + 2, " ");
      char *extraArg = strtok(NULL, " ");

      if (arg == NULL || extraArg != NULL) {
	char error_message[30] = "An error has occurred\n"; 
	write(STDERR_FILENO, error_message, strlen(error_message));
      } else {
	if (chdir(arg) != 0){
	  char error_message[30] = "An error has occurred\n"; 
	  write(STDERR_FILENO, error_message, strlen(error_message));
	}
      }
      printf("%s", prompt);
      continue;
    }
    runCmd(input);
    printf("%s", prompt);
  }

  free(input);
}

int main(int argc, char *argv[]) {
  if (argc == 1) {
    processLines("wish> ", stdin);
  } else if (argc == 2) {
    FILE *file = fopen(argv[1], "r");
    if (file == NULL) {
      exit(1);
    }
    processLines("", file);
  } else {
    exit(1);
  }
  return 0;
}  
