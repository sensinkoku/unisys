#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdint.h>
#include "centry.h"
//declare static functions
static void insert_centry_list(struct c_entry * head, struct c_entry * insert);

//define extern functions
struct c_entry* make_new_client(struct c_entry * head, uint32_t id, uint32_t ip, uint32_t mask, short status, int ttl) {
  struct c_entry * newce = (struct c_entry *) malloc(sizeof(struct c_entry));
  newce->fp = newce;
  newce->bp = newce;
  newce->id.s_addr = id;
  newce->cli_addr.s_addr = ip;
  newce->netmask.s_addr = mask;
  newce->stat = stat;
  newce->ttl = ttl;
  newce->ttlcounter = ttl;
  insert_centry_list(head, newce);
  return newce;
}
struct c_entry * search_client (struct c_entry *head, uint32_t id) {
  struct c_entry * p;
  p = head->fp;
  while (p != head) {
    if (p->id.s_addr == id) {
      return p;
    }
    p = p->fp;
  }
    return NULL;
}
/*int rm_client(struct c_entry * head, uint32_t id) {
  struct c_entry * p;
  p = head->fp;
  while (p != head) {
    if (p->id.s_addr == id) {
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
}*/
int rm_client(struct c_entry *c) {
  c->fp->bp = c->bp;
  c->bp->fp = c->fp;
  c->fp = NULL;
  c->bp = NULL;
  fprintf(stderr, "Delete client. id:%d\n", c->id.s_addr);
  free(c);
  return 0;
}

int init_head_struct(struct c_entry* head, uint32_t id) {
  head->fp = head;
  head->bp = head;
  head->id.s_addr = 0;
  head->cli_addr.s_addr = 0;
  head->mask.s_addr = 0;
  return 0;
}
int extent_ttl (struct c_entry * c, uint16_t ttl)  {
  c->ttl = ttl;
  c->ttlcounter = ttl;
}
int print_client_list(struct c_entry * head) {
  struct c_entry * p;
  p = head->fp;
  while (p != head) {
    printf("ip:%d mask:%d\n", p->id.s_addr, p->mask.s_addr);
    p = p->fp;
  }
  return 0;
}
int client_status_change (struct c_entry *c, int from, int to) {
  if (c->stat != from) {
    fprintf(stderr, "stat transition miss\n");
    return -1;
  }
  char fromc[32];
  char toc[32];
  switch(from){
    case STAT_WAIT_DISCOVER:
      fromc = "STAT_WAIT_DISCOVER\0";
    case STAT_WAIT_REQUEST:
      fromc = "STAT_WAIT_REQUEST\0";
    case STAT_IP_ASSIGNED:
      fromc = "STAT_IP_ASSIGNED\0";
    case STAT_WAIT_REQUEST_2:
      fromc = "STAT_WAIT_REQUEST_2\0"
    default:
  }
  c->stat = to;
  return 0;
}

int search_ttl_and_decrease_time(struct c_entry *head) {
  struct c_entry *p;
  p = head->fp;
  while (p != head) {
    (p->ttlcounter)--;
    if (p->ttlcounter <= 0) rm_client(p);
    p = p->fp;
  }
  return 0;
}

static void insert_centry_list(struct c_entry * head, struct c_entry * insert) {
  insert->fp = head->fp;
  insert->bp = head;
  head->fp->bp = insert;
  head->fp = insert;
}
