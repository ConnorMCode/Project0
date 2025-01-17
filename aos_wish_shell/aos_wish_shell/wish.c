#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_PATH 10
#define MAX_PATH_LENGTH 256

char *path[MAX_PATH] = {NULL};

void updatePath(char *input) {
  for (int i = 0; i < MAX_PATH; i++) {
    if (path[i] != NULL){
      free(path[i]);
      path[i] = NULL;
    }
  }

  if (input == NULL || *input == '\0'){
    return;
  }

  char *saveptr = NULL;
  char *dir = strtok_r(input, " ", &saveptr);
  int index = 0;

  while (dir != NULL && index < MAX_PATH){
    if (dir[0] != '/'){
      char fullPath[256] = "./";
      strcat(fullPath, dir);
      path[index++] = strdup(fullPath);
    }else {
      path[index++] = strdup(dir);
    }
    dir = strtok_r(NULL, " ", &saveptr);
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

  if (cmd == NULL || strlen(cmd) == 0){
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
    return -1;
  }
  
  char *redirection = strstr(cmd, ">");
  char *outputFile = NULL;

  if (redirection != NULL) {
    *redirection = '\0';
    redirection++;
    outputFile = strtok_r(redirection, " ", &saveptr);

    if (outputFile == NULL || strtok_r(NULL, " ", &saveptr) != NULL){
      char error_message[30] = "An error has occurred\n";
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
  
  if (args[0] == NULL){
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
    return -1;
  }

  for (int i = 0; path[i] != NULL; i++){
    strcpy(cmdPath, path[i]);
    if (cmdPath[strlen(cmdPath) - 1] != '/') {
      strcat(cmdPath, "/");
    }
    strcat(cmdPath, args[0]);

    if (access(cmdPath, X_OK) == 0){
      pid = fork();

      if (pid == 0){
	if (outputFile != NULL){
	  int out = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
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
	return pid;
      }
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
      break;
    }
    if (strncmp(input, "path", 4) == 0){
      char *arg = input + 4;
      while (*arg == ' ') arg++;
      if (*arg == '\0'){
	updatePath(NULL);
      } else {
	updatePath(arg);
      }
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

    char *cmds[256];
    int cmdCount = 0;
    char *saveptr = NULL;

    char *cmd = strtok_r(input, "&", &saveptr);
    while (cmd != NULL) {
      cmds[cmdCount++] = cmd;
      cmd = strtok_r(NULL, "&", &saveptr);
    }

    pid_t pids[cmdCount];
    int pidCount = 0;

    for(int i = 0; i < cmdCount; i++){
      char *singleCmd = cmds[i];
      while (*singleCmd == ' ') singleCmd++;
      if (*singleCmd != '\0') {
	pid_t pid = runCmd(singleCmd);
	if (pid > 0){
	  pids[pidCount++] = pid;
      }
    }
  }

    for (int i = 0; i < pidCount; i++){
      waitpid(pids[i], NULL, 0);
    }

    printf("%s", prompt);
  }
  free(input);
}

int main(int argc, char *argv[]) {

  updatePath("/bin");
  
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
