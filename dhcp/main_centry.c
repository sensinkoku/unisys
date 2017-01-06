#include "centry.h"
#include <stdio.h>
#include <stdlib.h>
int main(int argc, char *argv[])
{
  struct c_entry head;
  init_head_struct(&head, 0, 0);
  make_new_client(&head, 1, 1, 1, 1);
  make_new_client(&head, 2, 2, 2, 2);
  if (search_client(&head, 1, 1) == NULL) {
    printf("null\n");
  }
  int i;
  i = rm_client(&head, 1, 1);
  printf("%d\n", i);
  print_client_list(&head);
  return 0;
}
