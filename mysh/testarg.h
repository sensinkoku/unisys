#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#define LEN 100
void myinput(int* ac, char ** av, char * input);
int main(int argc, char *argv[])
{
  int ac;
  char *av[16];
  char *input;
  int status;
  while(1) {
    printf("% ");
    myinput(&ac, av, input);
    pid_t p = fork();
    if (p == 0) {
      char * env[2];
      env[0]="";
      env[1]=av[1];
      execvp(av[0],env);
    } else {
      wait(&status);
    }
    
  }
  return 0;
}

void myinput(int* ac, char ** av, char * input) {
  fgets(input, 256, stdin);
  input[strlen(input) - 1] = '\0';
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
