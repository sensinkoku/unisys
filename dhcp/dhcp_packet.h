#ifndef DHCP_PACKET_H_
#define DHCP_PACKET_H_
#include <stdint.h>
// messages
#define DHCPDISCOVER 1
#define DHCPOFFER 2
#define DHCPREQUEST 3
#define DHCPACK 4
#define DHCPRELEASE 5

struct dhcp_packet{
	uint8_t type;
	uint8_t code;
	uint16_t time;
	uint32_t address;
	uint32_t netmask;
};
extern void init_dhcp_packet(struct dhcp_packet * p,uint8_t type, uint8_t code, uint16_t time, uint32_t ip, uint32_t mask);
extern void print_dhcp_packet(struct dhcp_packet *p, int i); //i = 0 receice,  i = 1 send
#endif //DHCP_PACKET_H_
