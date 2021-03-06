#ifndef IP_LIST_H_
#define IP_LIST_H_
#include <stdint.h>
struct ip_list{
  // Bi-directional pointers
  struct ip_list * fp;
  struct ip_list * bp;
  // ips hostorder
  uint32_t ip;
  uint32_t mask;
};
extern int init_ip_struct(struct ip_list * il, uint32_t ip, uint32_t mask);
extern int init_ip_list_from_arg(struct ip_list * head, char * filename);
extern int add_new_ip(struct ip_list *head, uint32_t ip, uint32_t mask);
extern int getrm_ip_from_list(struct ip_list * head, struct ip_list **p, int ipttl);
extern void print_ip_list(struct ip_list * hi);
extern int add_new_ip_print(struct ip_list * head, uint32_t ip, uint32_t mask);
//extern int ip_list* rm_ip_from_list();

#endif //IP_LIST_H_
