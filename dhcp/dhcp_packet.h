#ifndef DHCP_PACKET_H_
#define DHCP_PACKET_H_
// messages
#define DHCPDISCOVER 1
#define DHCPOFFER 2
#define DHCPREQUEST 3
#define DHCPREPLY 4
#define DHCPRELEASE 5

struct dhcp_packet{
  //ips
  //ports
  //messages type, code, tyme to live, ip, netmask
};
extern struct dhcp_packet* make_dhcp_packet();

#endif //DHCP_PACKET_H_
