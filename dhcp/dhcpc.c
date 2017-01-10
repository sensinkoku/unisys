#include "dhcpc.h"
#include "dhcp_packet.h"

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
//#include <errno>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>

#define DESTINATION_PORT 51230

#define STAT_INITIAL 0
#define STAT_WAIT_OFFER 1
#define STAT_WAIT_ACK 2
#define STAT_HAVE_IP 3

#define CODE_IN_REQUEST_FIRST 2
#define CODE_IN_REQUEST_EXTEND 3

static uint32_t get_ip_from_arg(int argc, char * argv[]);
static int init_dhcpc_struct(struct dhcpc * dhc, uint32_t ip);

static int send_discover(struct dhcpc * dhc);

static int dhcpc_loop(struct dhcpc * dhc);
static int wait_server_message(struct dhcpc * dhc);
static int set_signal_and_timer ();

static int status_change(struct dhcpc * dhc, int to);
static int msg_offer(struct dhcpc * dhc);
static int msg_ack(struct dhcpc * dhc);
static int msg_extend_ack(struct dhcpc * dhc);
static int recv_packet(struct dhcpc * dhc);

//debug, print, error



//define extern functions
int init_dhcpc(struct dhcpc * dhc, int argc, char * argv[]) {
	uint32_t ip;// host_order
	ip = get_ip_from_arg(argc, argv);
	init_dhcpc_struct(dhc, ip);
	dhcpc_loop(dhc);
	return 0;
}
static int recv_packet(struct dhcpc * dhc) {
		int count;
		socklen_t sktlen;
		sktlen = sizeof dhc->skt;
		if ((count  = recvfrom (dhc->s, dhc->buf, sizeof (struct dhcp_packet), 0, (struct sockaddr *)&(dhc->skt), &sktlen)) < 0) {
			perror ("recvfrom error");
			exit(1);
		}
		return 0;
}
static int dhcpc_loop(struct dhcpc * dhc) {
	send_discover (dhc);
	for (;;) {
		recv_packet(dhc);
		switch(dhc->stat){
			case STAT_WAIT_OFFER:
				msg_offer(dhc);
				break;
			case STAT_WAIT_ACK:
				msg_ack(dhc);
				break;
			case STAT_HAVE_IP:
				msg_extend_ack(dhc);
				break;
			default:
				fprintf(stderr, "Type is not set\n");
				exit;
			break;
		}

	}
}
static int send_discover(struct dhcpc * dhc) {
	struct dhcp_packet packet;
	init_dhcp_packet(&packet, DHCPDISCOVER, 0,0,0,0);
	if (sendto(dhc->s, &packet, sizeof(struct dhcp_packet), 0, (struct sockaddr *)&(dhc->skt), sizeof dhc->skt) < 0) {
		perror("sendto");
		exit(1);
		return -1;
	}
	fprintf(stderr, "send DISCOVER\n");
	status_change(dhc, STAT_WAIT_OFFER);
	return 0;
}

static int init_dhcpc_struct(struct dhcpc * dhc, uint32_t ip) {
	if ((dhc->s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror ("client socket open error");
		exit(1);
	}
	/*dhc->myskt.sin_family = AF_INET;
	dhc->myskt.sin_port = htons();
	dhc->myskt.sin_addr.s_addr = htol(INADDR_ANY);
	int count;
		if ((count = bind(dhc->s, (struct sockaddr *)&(dhc->myskt), sizeof dhc->myskt)) < 0) {
		perror("client bind error");
		exit(1);
	}
*/


	uint16_t port = DESTINATION_PORT;
	dhc->skt.sin_family = AF_INET;
	dhc->skt.sin_port = htons(port); //network order
	dhc->skt.sin_addr.s_addr = htonl(ip); //network order
	dhc->ttlcounter = 100;
	dhc->stat = STAT_INITIAL;
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
		fprintf(stderr, "Server IP from arg is %s\n", ip);
		inet_aton(ip, &ips);
		return ips.s_addr;
	}
}

static int status_change(struct dhcpc * dhc, int to) {
	fprintf(stderr, "STATE CHANGED \n");
	dhc->stat = to;
	return 0;
}
static int set_signal_and_timer () {
	struct itimerval timer;
	timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 1000000;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 1000000;
	//sigaction(SIGALRM, search_ttl_and_decrease_time);
 	/*if (setitimer(ITIMER_REAL, &timer, NULL) < 0) {
 		perror("setitimer error");
 		exit(1);
 	}*/
}
static int msg_offer (struct dhcpc * dhc){
	struct dhcp_packet packet;
	if (dhc->buf->type == DHCPOFFER) {
		if (dhc->buf->code != 0) {
			fprintf(stderr, "DHCPOFFER says no available IP in server\n");
			return -1;
		} else {
			fprintf(stderr, "Received packet. message:DHCPOFFER\n");
			uint16_t time = dhc->buf->time;
			uint32_t ip = dhc->buf->address;
			uint32_t mask = dhc->buf->netmask; 
			init_dhcp_packet(&packet, DHCPREQUEST, CODE_IN_REQUEST_FIRST, time, ip, mask);
			int count;
			if ((count = sendto(dhc->s, &packet, sizeof(struct dhcp_packet), 0, (struct sockaddr *)&(dhc->skt), sizeof dhc->skt)) < 0) {
					perror("sendto");
					exit(1);
			}
			fprintf(stderr, "send REQUEST\n");
			status_change(dhc, STAT_WAIT_ACK);
			return 0;
		}	
	} else {
		fprintf(stderr, "Message error: state is not true, should be DHCPOFFER\n");
		exit(1);
		return -1;
	}
}
static int msg_ack (struct dhcpc * dhc){
	struct dhcp_packet packet;
	if (dhc->buf->type == DHCPACK) {
		if (dhc->buf->code != 0) {
			fprintf(stderr, "ACK msg refused IP assignment. REQUEST Message was illegal.\n");
			return -1;
		} else {
			fprintf(stderr, "Received packet. message:DHCPACK\n");
			uint16_t time = dhc->buf->time;
			uint32_t ip = dhc->buf->address;
			uint32_t mask = dhc->buf->netmask; 
			dhc->cli_addr.s_addr = ip;//network order
			dhc->netmask.s_addr = mask; //network order
			dhc->ttl = dhc->buf->time;
			dhc->ttlcounter = dhc->ttl;
			dhc->ipsetor = 1;
			ip = ntohl(ip);
			mask = ntohl(ip);
			struct in_addr in;
			in.s_addr = ip;
			char * ipstring = inet_ntoa(in);
			in.s_addr = mask;
			char * maskstring = inet_ntoa(in);
			fprintf(stderr, "Ip set. IP: %s Mask:%s\n", ipstring, maskstring);
			status_change(dhc, STAT_HAVE_IP);
			return 0;
		}
		
	} else {
		fprintf(stderr, "Message error: state is not true, should be DHCPACK\n");
		exit(1);
		return -1;
	}
}
static int msg_extend_ack (struct dhcpc * dhc){
	struct dhcp_packet packet;
	if (dhc->buf->type == DHCPACK) {
		if (dhc->buf->code != 0) {
			fprintf(stderr, "ACK msg refused IP assignment. REQUEST Message was illegal.\n");
			return -1;
		} else {
			fprintf(stderr, "Received packet. message:DHCPACK\n");
			uint16_t time = dhc->buf->time;
			uint32_t ip = dhc->buf->address;
			uint32_t mask = dhc->buf->netmask; 
			dhc->cli_addr.s_addr = ip;//network order
			dhc->netmask.s_addr = mask; //network order
			dhc->ttl = dhc->buf->time;
			dhc->ttlcounter = dhc->ttl;
			dhc->ipsetor = 1;
			ip = ntohl(ip);
			mask = ntohl(ip);
			struct in_addr in;
			in.s_addr = ip;
			char * ipstring = inet_ntoa(in);
			in.s_addr = mask;
			char * maskstring = inet_ntoa(in);
			fprintf(stderr, "Ip available time extended. IP: %s Mask:%s\n", ipstring, maskstring);
			return 0;
		}
		
	} else {
		fprintf(stderr, "Message error: state is not true, should be DHCPACK\n");
		exit(1);
		return -1;
	}
}


