#include "centry.h"
int main(int argc, char *argv[])
{
  struct c_entry head;
  init_head_struct(&head, 0, 0);
  make_new_client(&head, 1, 1, 1,1);
  make_new_client(&head, 2, 2, 2,2);
  search_client(&head, 1, 1);
  rm_client(&head, 1, 1);
  print_client_list(&head);
  return 0;
}
