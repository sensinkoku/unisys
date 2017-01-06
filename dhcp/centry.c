#include "centry.h"
#include <stdlib.h>
//declare static functions
static void insert_centry_list(struct c_entry * head, struct c_entry * insert);

//define extern functions
struct c_entry * make_new_client(struct c_entry * head, uint32_t ip, uint32_t portnum, short status, int lease) {
  struct c_entry * newce = (struct c_entry *) malloc(sizeof(struct c_entry *));
  newce->cli_addr.sin_addr = ip;
  newce->cli_port = portnum;
  newce->status = status;
  newce->lease = lease;
  insert_centry_list(head, newce);
  return newce;
}
struct c_entry * search_client (struct c_entry *head,uint32_t ip, uint16_t portnum) {
  struct c_entry * p;
  p = head->fp;
  while (p != head) {
    if (p->cli_addr.sin_addr == ip && p->cli_port == portnum) {
      return p;
    }
    p = p->fp;
  }
    return NULL;
}
int rm_clientt(struct c_entry * head, uint32_t ip, uint16_t portnum) {
  struct c_entry * p;
  p = head->fp;
  while (p != head) {
    if (p->cli_addr.sin_addr == ip && p->cli_port == portnum) {
      p->fp->bp = p->bp;
      p->bp->fp = p->fp;
      p->fp = NULL;
      p->bp = NULL;
      free(p);
      return 0;
    }
    p = p->fp;
  }
    return -1;
}
int init_head_struct(struct c_entry* head,uint32_t ip, uint16_t portnum) {
  head->fp = head;
  head->bp = head;
  head->cli_addr.sin_addr = 0;
  head->cli_port = 0;
  return 0;
}
static void insert_centry_list(struct c_entry * head, struct c_entry * insert) {
  insert->fp = head->fp;
  insert->bp = head;
  head->fp->bp = insert;
  head->fp = insert;
}
