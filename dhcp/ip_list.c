#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include "ip_list.h"


static int parse_ip_fromline(char * line, uint32_t * ip, uint32_t * mask);
static int insert_iplist(struct ip_list *beforep, struct ip_list *insertp);
// define extern functions
int init_ip_list_from_arg(struct ip_list * head, char * filename) {
  FILE * fp;
  char line[100];
  int i;
  uint32_t ip;
  uint32_t mask;
  if((fp = fopen(filename, "r")) == NULL) {
    fprintf (stderr, "IP list file not exist\n");
    exit(1);
    return -1;
  }
  while (fgets(line, 100, fp) != NULL) {
    //end of line is \n \0
    if (parse_ip_fromline(line, &ip, &mask) < 0) {
      fprintf(stderr, "input file error\n");
      exit(1);
      return -1;
    } else {
      add_new_ip(head, ip, mask);
    }
  }
  return 0;
}
int add_new_ip(struct ip_list * head, uint32_t ip, uint32_t mask) {
  //malloc and make ip_struct and add to list
  struct ip_list *p;
  p = head;
  while(p->fp != head) {
    p = p->fp;
  }
  struct ip_list * newip = (struct ip_list *)malloc(sizeof(struct ip_list));
  init_ip_struct(newip, ip, mask);
  insert_iplist(p, newip);
}

extern struct ip_list* getrm_ip_from_list(struct ip_list * head) {
  struct ip_list * p;
  p = head->fp;
  if (p == head) {
    fprintf (stderr, "No IP on list.\n");
    return NULL;
  } else {
  head->fp = p->fp;
  p->fp->bp = head;
  p->fp = NULL;
  p->bp = NULL;
  return p;
  }
}

extern void print_ip_list(struct ip_list * hi) {
  struct ip_list * p;
  p = hi;
  printf("ip:%d mask: %d\n", p->ip, p->mask);
  p = p->fp;
  while(p != hi) {
      printf("ip:%d mask: %d\n", p->ip, p->mask);
      p = p->fp;
  }
  return;
}
int init_ip_struct(struct ip_list * il, uint32_t ip, uint32_t mask) {
  il->fp = il;
  il->bp = il;
  il->ip = ip;
  il->mask = mask;
}
static int parse_ip_fromline(char * line, uint32_t * ip, uint32_t * mask) {
  char ip_char[32];
  char mask_char[32];
  struct in_addr buf;
  sscanf(line, "%s %s", ip_char, mask_char);
  inet_aton(ip_char, &buf);
  *ip = buf.s_addr;
  inet_aton(mask_char, &buf);
  *mask = buf.s_addr;
  return 0;
}
static int insert_iplist(struct ip_list *beforep, struct ip_list *insertp) {
  //
  beforep->fp->bp = insertp;
  insertp->fp = beforep->fp;
  beforep->fp = insertp;
  insertp->bp = beforep;
}
