#ifndef DHCPC_H_
#define DHCPC_H_
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdint.h>

#define DESTINATION_PORT 51230

#define STAT_INITIAL 0
#define STAT_WAIT_OFFER 1
#define STAT_WAIT_OFFER_2ND 2
#define STAT_WAIT_ACK 3
#define STAT_WAIT_ACK_2ND 4
#define STAT_IN_USE 5
#define STAT_WAIT_EXT_ACK 6


#define CODE_IN_REQUEST_FIRST 2
#define CODE_IN_REQUEST_EXTEND 3

#define PACKET_WAIT_TTL 10
//struct in_addr;
//Data structure
struct dhcpc{
	int s;
	struct sockaddr_in myskt;
	struct sockaddr_in skt;//server socket
	struct dhcp_packet *buf;

  	int stat; //status
  	int ttlcounter; //left time
  	int msgttlcounter;
  //below: network byte order
//  struct in_addr id; //idenfier
  //  struct in_addr addr;
  	int ipsetor; // 0 is not set, 1 is set already
  struct in_addr cli_addr;
  struct in_addr netmask;
  uint16_t ttl; // 10sec is tyme out for waiting message except DHCPDISCOVER
  uint16_t ipttl; //ip ttl;
};
//extern functions
extern int init_dhcpc(struct dhcpc* dhc, int argc, char * argv[]);
extern int loop_dhcpc(struct dhcpc * dhc);

extern struct c_entry* make_new_client(struct c_entry * head, uint32_t id, uint32_t ip, uint32_t mask, short stat, int ttl);
extern struct c_entry* search_client(struct c_entry * head, uint32_t id);
extern int rm_client(struct c_entry * c);
extern int extent_ttl(struct c_entry *c, uint16_t ttl);
extern int print_client_list(struct c_entry * head);
extern int client_status_change(struct c_entry *c, int to);
extern int search_ttl_and_decrease_time(struct c_entry *head);
#endif // DHCPC_H_

