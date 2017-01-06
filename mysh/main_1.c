#include <stdio.h>
#include "myinput.h"
int main(int argc, char *argv[])
{
  int ac;
  char *av[50];
  // char **av; is error in myinput
  
  char input[256];
  myinput(&ac, av, input);
  print_args(ac, av);
  return 0;
}
