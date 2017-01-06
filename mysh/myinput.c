#include "myinput.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
//define extern function
#define MAXLENGTH 150

void myinput(int* ac, char ** av, char * input) {
	fgets(input, 250, stdin);
  if (strlen(input)>=1) input[strlen(input) - 1] = '\0';
  char c;
  *ac = 0;
  int flagchar = 1;
  // when flagchar is 1, waiting new arg, else during arg
  int avnum = 0;
  int i;
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
void print_args(int ac, char **av) {
	int i;
	for (i = 0; i < ac; i++) {
		printf ("%d : %s\n", i, av[i]);
	}
	return;
}