#include "dhcpc.h"

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <errono>
#include <unistd.h>
#include <signal.h>

#define DESTINATION_PORT 51230

#define STAT_INITIAL 0
#define STAT_WAIT_OFFER 1
#define STAT_WAIT_ACK 2
#define STAT_HAVE_IP 3

static uint32_t get_ip_from_arg(int argc, char * argv[]);
static int init_dhcpc_struct(struct dhcpc * dhc, );
static int dhcp_client_work(struct dhcpc * dhc);

static int send_discover(dhcpc * dhc);
static int status_change(struct dhcpc * dhc, int from, int to);
static int msg_offer(struct dhcpc * dhc);
static int msg_ack(struct dhcpc * dhc);






//define extern functions
int init_dhcpc(struct dhcp_client * dhc, int argc, char * argv[]) {
	uint32_t ip// byte_order
	ip = get_ip_from_arg(argc, argv);
	init_dhcpc_struct(dhc, ip);
	dhcp_client_work(dhc);

	return 0;
}

static dhcp_client_work(struct dhcpc * dhc) {
	struct dhcp_packet t_packet;
	send_discover (dhc);
	for (;;) {
		
	
	
	}

}
static int send_discover(dhcpc * dhc) {
	struct dhcp_packet packet;
	init_dhcp_packet(&packet, DHCPDISCOVER, 0,0,0,0);
	if (sendto(dhc->s, &packet, sizeof(struct dhcp_packet), 0, dhc->(struct sockaddr *)&skt, sizeof skt) < 0) {
		perror("sendto");
		exit(1);
	}
	dhc->stat = STAT_WAIT_OFFER;
	return 0;
}

static int init_dhcpc_struct(struct dhcpc * dhc, uint32_t ip) {
	if ((dhc->s = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		perror ("client socket open error");
		exit(1);
	}
	uint16_t port = DESTINATION_PORT;
	dhc->skt.sin_family = AF_INET;
	dhc->sin_port = htons(port); //network order
	dhc->skt.sin_addr.s_addr = htonl(ip); //network order
	dhc->ttlcounter = 100;
	dhc->stat = INITIALSTAT;
	return 0;
}

static uint32_t get_ip_from_arg(int argc, char * argv[]) {
	struct in_addr ips;
	if (argc != 2) {
		fprintf(stderr, "Usage: ./dhcpd <fileneme>\n");
    	exit(1);
    	return 0;
	} else {
		char * ip = argv[1];
		inet_aton(ip, &ips);
		return ips.s_addr;
	}
}