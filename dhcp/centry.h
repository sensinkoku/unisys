#ifndef CENTRY_H_
#define CENTRY_H_
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include<stdint.h>
#define STAT_WAIT_DISCOVER 0
#define STAT_WAIT_REQUEST 1
#define STAT_IP_ASSIGNMENT 2
//struct in_addr;
//Data structure
struct c_entry{
  //Bi-directional pointers
  struct c_entry *fp;
  struct c_entry *bp;
  // left time
  int lease;
  //client ip, portnum, lease time, server status
  struct in_addr cli_addr;
  uint16_t cli_port;
  short status;
};
//extern functions
extern struct c_entry* make_new_client(struct c_entry * head, uint32_t ip, uint32_t portnum, short status, int lease);
extern struct c_entry* search_client(struct c_entry * head, uint32_t ip, uint16_t portnum);
extern int init_head_struct(struct c_entry*head,uint32_t ip, uint16_t portnum);
extern int rm_client(struct c_entry * head, uint32_t ip, uint16_t portnum);
extern int print_client_list(struct c_entry * head);
#endif // CENTRY_H_
