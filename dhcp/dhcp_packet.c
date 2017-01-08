#include "dhcp_packet.h"
#include <stdint.h>
struct void init_dhcp_packet(struct dhcp_packet * p,uint8_t type, uint8_t code, uint16_t time, uint32_t ip, uint32_t mask)
{
	p->type = type;
	p->code = code;
	p->time = time;
	p->address = ip;
	p->netmask = mask;
	return;
}