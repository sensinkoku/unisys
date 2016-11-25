#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
int main(int argc, char *argv[])
{
  int s, count, datalen;
  char sbuf[512];
  in_port_t port;
  int length;
  uint32_t portnum = 49152;
  char * ipad = "131.113.110.80";
  struct in_addr ipaddr;
  //  ipaddr.s_addr = (inet_aton(ipad, ));
  inet_aton(ipad, &ipaddr);
  struct sockaddr_in skt;
  if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket");
    exit(1);
  }
  fgets(sbuf, 512, stdin);
  length = strlen(sbuf);
  //sock addr in is ip is struct in_addr
  //addr in is inaddr_t 32bit unsigned
  //portnum is in_port_t
  skt.sin_family = AF_INET;
  skt.sin_port = htons(portnum);
  skt.sin_addr.s_addr = htonl(ipaddr.s_addr);
  if ((count = sendto(s, sbuf, length, 0, (struct sockaddr * )&skt, sizeof skt)) < 0) {
      perror("sendto");
      exit(1);
}
  return 0;
}
