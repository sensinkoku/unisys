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
  char rbuf[512];
  in_port_t port;
  int length, r_length;
  uint32_t portnum = 49152;
  char * ipad = "131.113.110.80";
  int backlog = 5;
  struct in_addr ipaddr;
  //  ipaddr.s_addr = (inet_aton(ipad, ));
  inet_aton(ipad, &ipaddr);
  struct sockaddr_in skt, myskt;
  socklen_t addrlen;
  if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    exit(1);
  }
  /* if (listen(s, backlog) < 0) {
    perror("listen");
    exit(1);
  }
   if (accept(s, &myskt,&addrlen) < 0) {
    perror("accept");
    exit(1);
  } */
  fgets(sbuf, 512, stdin);
  length = strlen(sbuf);
  //sock addr in is ip is struct in_addr
  //addr in is inaddr_t 32bit unsigned
  //portnum is in_port_t
  memset(&skt, 0, sizeof skt);
  skt.sin_family = AF_INET;
  skt.sin_port = htons(portnum);
  skt.sin_addr.s_addr = ipaddr.s_addr;
  if (count = (connect(s, (struct sockaddr *)&skt, sizeof skt)) < 0) {
    perror("listen");
    exit(1);
  }
  if ((count = send(s, sbuf, length, 0)) < 0) {
      perror("sendt");
      exit(1);
      }
  if (recv(s, rbuf, r_length, 0) < 0) {
    perror("listen");
    exit(1);
  }
  close(s);
  return 0;
}
