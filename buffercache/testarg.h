#include <stdio.h>
#include <string.h>
#define LEN 100
void myinput(int* ac, char ** av, char * input);
/*int main(int argc, char *argv[])
{
	int ac;
	char *av[16];
	char inp[100];
	myinput(&ac, av, inp);*/
  /*int ac;
  char *av[16];
  char *input;
  int i;
  fgets(input, 256, stdin);
  input[strlen(input) - 1] = '\0';
  char c;
  ac = 0;
  int n;
  int flagchar = 1;
  int avnum = 0;
  for (c = input[0]; c != '\0'; *input++) {
  	c = input[0];
  	printf("%c\n", c);
    if (avnum > 16) break;
    if(c != ' ' && c != '\t' && flagchar == 1) {
      av[avnum] = input;
      avnum++;
      ac++;
      flagchar = 0;
    } else if (flagchar == 0 && (c == ' ' || c == '\t')) {
	     input[0] = '\0';
       flagchar = 1;
    }
  }*/
  /*printf("%d\n", ac);
  int i;
  for (i = 0; i < ac; i++) {
  	printf("%s\n", av[i]);
  }
 return 0;
}*/
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