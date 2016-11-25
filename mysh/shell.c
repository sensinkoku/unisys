#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#define LEN 100
void myinput(int* ac, char ** av, char * input, int *ispipe, int *isredirect);
int main(int argc, char *argv[])
{
  int ac;
  char *av[16];
  char input[256];
  int ispipe[ac];
  int isredirect[ac];
  int status;
  int i;
  while(1) {
    printf("$ ");
    myinput(&ac, av, input, ispipe, isredirect);
    if (ac >=1){
      if (strcmp(av[0],"cd") == 0) {
        char *  home = getenv("HOME");
        if (ac == 1 || strcmp(av[1], "~") == 0) chdir(home);
        else chdir(av[1]);
      } else if (strcmp(av[ac-1],"exit")) {
	exit(0);
      } else {
	int i;
	int pipenum = 0;
	int pipeplace;
	int *pipefile[pipenum];
	for (i = 0; i < ac; i++) if (ispipe[i] == 1) pipenum++;
	if (ispipe[0] != 0 || ispipe[ac] != 0) fprintf(stderr, "Input error\n");
	//change input and output stream
	for (i = 0; i < pipenum; i++) pipe(pipefile[i]);
	for (i = 0; i < ac-1; i++) {
	  if (pipenum == 0) {

	  }
	  //	  while (ispipe[i] != 1) if (i != )i++;
	  
	}
        pid_t p = fork();
        if (p == 0) {
          if (ac >= 1) av[ac] = NULL;
          execvp(av[0],av);
        } else {
          if (strcmp(av[ac-1],"&") != 0) {
            wait(&status);
          }
        }
      }
    }
  }
  return 0;
}

void myinput(int* ac, char ** av, char * input, int *ispipe, int *isredirect) {
  fgets(input, 250, stdin);
  if (strlen(input)>=1) input[strlen(input) - 1] = '\0';
  char c;
  *ac = 0;
  int flagchar = 1;
  int avnum = 0;
  for (c = input[0]; c != '\0'; *input++) {
  	c = input[0];
    if (avnum > 16) break;
    if(c != ' ' && c != '\t' && flagchar == 1) {
      av[avnum] = input;
      avnum++;
      (*ac)++;
      flagchar = 0;
    } else if (flagchar == 0 && (c == ' ' || c == '\t')) {
	     input[0] = '\0';
       flagchar = 1;
    }
  }
  int i;
  for (i = 0; i < *ac; i++) {
    ispipe[i] = 0;
    isredirect[i] = 0;
  }
  for (i = 0; i < *ac; i++) {
    if (strcmp(av[i],"|")) ispipe[i] = 1;
    if (strcmp(av[i], ">")) ispipe[i] = 2;
    if (strcmp(av[i], "<")) ispipe[i] = 3;
  }
  return;
}
