#include <stdio.h>
#include "ip_list.h"
int main(int argc, char *argv[])
{
  struct ip_list  ih; //this is not pointer
  init_ip_struct(&ih, 0, 0);
  init_ip_list_from_arg(&ih, argv[1]);
  print_ip_list(&ih);
  return 0;
}
