#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#define LEN 100
void myinput(int* ac, char ** av, char * input);
int main(int argc, char *argv[])
{
  int ac;
  char *av[16];
  char input[256];
  int status;
  int i;
  while(1) {
    printf("$ ");
    myinput(&ac, av, input);
    if (ac >=1){
      if (strcmp(av[0],"cd") == 0) {
        char *  home = getenv("HOME");
        if (ac == 1 || strcmp(av[1], "~") == 0) chdir(home);
        else chdir(av[1]);
      } else {
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

void myinput(int* ac, char ** av, char * input) {
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
  return;
}