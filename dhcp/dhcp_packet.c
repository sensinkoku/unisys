#include "dhcp_packet.h"
#include <stdint.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
void init_dhcp_packet(struct dhcp_packet * p,uint8_t type, uint8_t code, uint16_t time, uint32_t ip, uint32_t mask)
{
	p->type = type;
	p->code = code;
	p->time = time;
	p->address = ip;
	p->netmask = mask;
	return;
}
void print_dhcp_packet(struct dhcp_packet *p, int i) {
  char stat[32];
  char msgd[32] = "DHCPDISCOVER\0";
  char msgo[32] = "DHCPOFFER\0";
  char msgr[32] = "DHCPREQUEST\0";
  char msga[32] = "DHCPACK\0";
  char msgrl[32] ="DHCPRELEASE\0";
  switch(p->type){
  case DHCPDISCOVER:
    strncpy(stat, msgd, 30);
    break;
  case DHCPOFFER:
    strncpy(stat, msgo, 30);
    break;
  case DHCPREQUEST:
    strncpy(stat, msgr, 30);
    break;
  case DHCPACK:
    strncpy(stat, msga, 30);
    break;
  case DHCPRELEASE:
    strncpy(stat, msgrl, 30);
    break;
  }
  
  if (i == 0) fprintf(stderr, "\nRECEIVED PACKET\n");
  else if (i == 1) fprintf(stderr, "\nSEND PACKET\n");
  fprintf(stderr, "//////////////////////\n");
  fprintf(stderr, "packet detail\n");
  fprintf(stderr, "TYPE : %s\n", stat);
  fprintf(stderr, "CODE : %d\n", (int)p->code);
  fprintf(stderr, "TIME : %d\n", (int)p->time);
  uint32_t ip, mask;
  struct in_addr ipin;
  ipin.s_addr = ntohl(p->address);
  char * ipstring = inet_ntoa(ipin);
  fprintf(stderr, "IP ADDRESS : %s\n",ipstring);
  struct in_addr maskin;
  maskin.s_addr = ntohl(p->netmask);
  char * maskstring = inet_ntoa(maskin);
  fprintf(stderr, "MASK : %s\n",maskstring);
  fprintf(stderr, "//////////////////////\n\n");
    return;
}
