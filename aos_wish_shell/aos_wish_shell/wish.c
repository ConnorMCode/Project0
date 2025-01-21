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

//Handles path string replacement
void updatePath(char *input) {
  //Clear path as input always full replaces
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

  //Seperate input by spaces and add to path
  while (dir != NULL && index < MAX_PATH){
    if (dir[0] != '/'){
      char fullPath[256] = "./"; //add ./ for local paths
      strcat(fullPath, dir);
      path[index++] = strdup(fullPath);
    }else {
      path[index++] = strdup(dir);
    }
    dir = strtok_r(NULL, " ", &saveptr);
  }
}

//Used to run non built-in commands, also handles redirection
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

  //check for redirection symbol
  char *redirection = strstr(cmd, ">");
  char *outputFile = NULL;

  //save specified output file name
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

  //divide input into array of arguments
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

  //loop through path attempting to access specified command in each directory
  for (int i = 0; path[i] != NULL; i++){
    strcpy(cmdPath, path[i]);
    if (cmdPath[strlen(cmdPath) - 1] != '/') {
      strcat(cmdPath, "/");
    }
    strcat(cmdPath, args[0]);

    //run command if accessable
    if (access(cmdPath, X_OK) == 0){
      //if output file specified, redirect output there
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

      //otherwise just execute command
      execv(cmdPath, args);
      char error_message[30] = "An error has occurred\n"; 
      write(STDERR_FILENO, error_message, strlen(error_message));
      exit(EXIT_FAILURE);
    }
  }
  char error_message[30] = "An error has occurred\n"; 
  write(STDERR_FILENO, error_message, strlen(error_message));
  return -1;
}

//interpret lines entered in wish prompt or in batch file
void processLines(char *prompt, FILE *src) {
  char *input = NULL;
  size_t len = 0;

  //prompt left blank for batch processing
  printf("%s", prompt);
  while (getline(&input, &len, src) != -1) {

    input[strcspn(input, "\n")] = 0;

    //Built-in commands exit, path, and cd
    if (strcmp(input, "exit") == 0){
      break;
    }
    //clear 'path' and leading spaces from path call and send to updatePath
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
    //changes directory to specified argument if it exists and if it isn't provided with extra arguments
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

    //non built-in commands checked for parallel & specifier then fed to runCmd
    char *cmd = strtok_r(input, "&", &saveptr);
    while (cmd != NULL) {
      cmds[cmdCount++] = cmd;
      cmd = strtok_r(NULL, "&", &saveptr);
    }

    pid_t pids[cmdCount];
    int pidCount = 0;

    //loop through cmds calling runCmd in each in parallel
    for(int i = 0; i < cmdCount; i++){
      char *singleCmd = cmds[i];
      while (*singleCmd == ' ') singleCmd++;
      if (*singleCmd != '\0') {
	pid_t pid = fork();
	if (pid == 0){
	  runCmd(singleCmd);
	  exit(EXIT_SUCCESS);
	} else if (pid > 0){
	  pids[pidCount++] = pid;
	} else{
	  char error_message[30] = "An error has occurred\n";
	  write(STDERR_FILENO, error_message, strlen(error_message));
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

  //pass /bin dynamically as path is freed every update
  updatePath("/bin");

  //send wist prompt to processLines if batch file not given, otherwise give empty prompt
  if (argc == 1) {
    processLines("wish> ", stdin);
  } else if (argc == 2) {
    FILE *file = fopen(argv[1], "r");
    if (file == NULL) {
      char error_message[30] = "An error has occurred\n";
      write(STDERR_FILENO, error_message, strlen(error_message));
      exit(1);
    }
    fseek(file, 0, SEEK_END);
    if (ftell(file) == 0){
      char error_message[30] = "An error has occurred\n";
      write(STDERR_FILENO, error_message, strlen(error_message));
      fclose(file);
      exit(1);
    }
    rewind(file);
    processLines("", file);
    fclose(file);
  } else {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
    exit(1);
  }
  return 0;
}  
