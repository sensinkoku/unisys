#ifndef DHCPD_H_
#define DHCPD_H_
#include <sys/types.h>
#include <sys/socket.h>
#include "dhcpd.h"
#include "centry.h"
#include "ip_list.h"
#include "dhcp_packet.h"

struct dhcp_packet;

#define DHCPSERVERPORT 51230
#define REQUEST_WAIT_TIME 10
//Forward declaration
struct in_addr;
struct ip_list;
struct sockaddr_in;
struct c_entry;
struct ip_list;

struct dhcpd {
  int s;  //server socket data
  int ipttl;
  //network order
  struct sockaddr_in myskt; //server ip
  struct dhcp_packet *buf; // late received packet
  struct sockaddr_in bufskt;  // late client socket
  struct c_entry c_entry_head; //client list head
  struct ip_list ip_list_head; //ip list head
};
//extern functions
extern void init_dhcpd();
extern void loop_dhcpd();
#endif // DHCPD_H_
