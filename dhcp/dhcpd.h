#ifndef DHCPD_H_
#define DHCPD_H_
#include <sys/type.h>
#include <sys/socket.h>

#define DHCPSERVERPORT 51230
//Forward declaration
struct in_addr;
struct ip_list;
// Data structure
struct dhcpd {
  //server socket data
  int s;
  struct sockaddr_in myskt;
  // buffers
  char buf[512];
  struct sockaddr_in bufskt;  
  //client struct list
  struct c_entry c_entry_head;
  //ip list
  struct ip_list ip_list_head;
};
//extern functions
extern void init_dhcpd();
extern void loop_dhcpd();
#endif // DHCPD_H_
