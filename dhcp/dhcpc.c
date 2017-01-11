#include "dhcpc.h"
#include "dhcp_packet.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <stdint.h>
//#include <errno>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <errno.h>

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

static int set_signal_and_timer() ;
static void signal_alrm();
static void signal_hup();
static void signal_alrm_etc(struct dhcpc *dhc);
static void alarm_work_dhcpc(struct dhcpc * dhc);

//debug, print, error
static flag_alrm;
static flag_hup;


//define extern functions
int init_dhcpc(struct dhcpc * dhc, int argc, char * argv[]) {
	uint32_t ip;// host_order
	ip = get_ip_from_arg(argc, argv);
	init_dhcpc_struct(dhc, ip);
	set_signal_and_timer();
	dhcpc_loop(dhc);
	return 0;
}
static int recv_packet(struct dhcpc * dhc) {
		int fds;
		int count;
		fd_set rdfds;
		FD_ZERO(&rdfds);
		FD_SET(dhc->s, &rdfds);
		socklen_t sktlen;
		sktlen = sizeof dhc->skt;
		fds = select((dhc->s)+1, &rdfds, NULL, NULL, NULL);
		if (errno == 4) {
			fprintf(stderr,"/sec");
			signal_alrm_etc(dhc);
			errno = 0;
			return -1;
		} else if ((count  = recvfrom (dhc->s, dhc->buf, sizeof (struct dhcp_packet), 0, (struct sockaddr *)&(dhc->skt), &sktlen)) < 0) {
			perror ("recvfrom error");
			exit(1);
		}
		return 0;
}
static int dhcpc_loop(struct dhcpc * dhc) {
	send_discover (dhc);
	for (;;) {
		int i;
		if((i = recv_packet(dhc)) < 0) continue;
		switch(dhc->stat){
			case STAT_WAIT_OFFER:
				msg_offer(dhc);
				break;
			case STAT_WAIT_ACK:
				msg_ack(dhc);
				break;
			case STAT_IN_USE:
				msg_extend_ack(dhc);
				break;
			// not yet done change
			case STAT_WAIT_OFFER_2ND:
				msg_offer(dhc);
				break;
			case STAT_WAIT_ACK_2ND:
				msg_extend_ack(dhc);
				break;
			case STAT_WAIT_EXT_ACK:
				msg_ack(dhc);
				break;
			default:
				fprintf(stderr, "Type is not set\n");
				exit(1);
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
	//	fprintf(stderr, "send DISCOVER\n");
	print_dhcp_packet(&packet, 1);
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
	dhc->buf = (struct dhcp_packet *)malloc(sizeof(struct dhcp_packet));
	dhc->skt.sin_family = AF_INET;
	dhc->skt.sin_port = htons(port); //network order
	dhc->skt.sin_addr.s_addr = htonl(ip); //network order
	dhc->ttlcounter = PACKET_WAIT_TTL;
	dhc->ttl = PACKET_WAIT_TTL;
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
    char fromc[32];
  char msgi[32] = "STAT_INITIAL\0";
  char msgo[32] = "STAT_WAIT_OFFER\0";
  char msgo2[32] = "STAT_WAIT_OFFER_2ND\0";
  char msga[32] = "STAT_WAIT_ACK\0";
  char msga2[32] = "STAT_WAIT_ACK_2ND\0";
  char msgh[32] = "STAT_IN_USE\0";
  char msgw[32] = "STAT_WAIT_EXT_ACK\0";

  char toc[32];
  switch(dhc->stat){
 	case STAT_INITIAL:
      strncpy(fromc,msgi,30);
      break;
 	case STAT_WAIT_OFFER:
      strncpy(fromc, msgo, 30);
      break;
    case STAT_WAIT_OFFER_2ND:
      strncpy(fromc, msgo2, 30);
      break;
    case STAT_WAIT_ACK:
      strncpy(fromc, msga, 30);
      break;
    case STAT_WAIT_ACK_2ND:
      strncpy(fromc, msga2, 30);
      break;
    case STAT_IN_USE:
      strncpy(fromc, msgh, 30);
      break;
     case STAT_WAIT_EXT_ACK:
     	strncpy(fromc, msgw, 30);
    default:
      break;
  }
  switch(to){
 	case STAT_INITIAL:
      strncpy(toc,msgi,30);
      break;
 	case STAT_WAIT_OFFER:
      strncpy(toc, msgo, 30);
      break;
    case STAT_WAIT_OFFER_2ND:
      strncpy(toc, msgo2, 30);
      break;
    case STAT_WAIT_ACK:
      strncpy(toc, msga, 30);
      break;
    case STAT_WAIT_ACK_2ND:
      strncpy(toc, msga2, 30);
      break;
    case STAT_IN_USE:
      strncpy(toc, msgh, 30);
      break;
     case STAT_WAIT_EXT_ACK:
     	strncpy(toc, msgw, 30);
    default:
      break;
  }
    fprintf(stderr ,"STATUS CHANGE  FROM:%s  TO:%s\n", fromc , toc);
    dhc->stat = to;
	return 0;
}
static int msg_offer (struct dhcpc * dhc){
	struct dhcp_packet packet;
	if (dhc->buf->type == DHCPOFFER) {
		if (dhc->buf->code != 0) {
			fprintf(stderr, "DHCPOFFER says no available IP in server\n");
			return -1;
		} else {
		  //			fprintf(stderr, "Received packet. message:DHCPOFFER\n");
			dhc->cli_addr.s_addr = ntohl(dhc->buf->address);
			dhc->netmask.s_addr = ntohl(dhc->buf->netmask);
		    print_dhcp_packet(dhc->buf, 0);
			uint16_t time = dhc->buf->time;
			uint32_t ip = dhc->buf->address;
			uint32_t mask = dhc->buf->netmask; 
			init_dhcp_packet(&packet, DHCPREQUEST, CODE_IN_REQUEST_FIRST, time, ip, mask);
			int count;
			if ((count = sendto(dhc->s, &packet, sizeof(struct dhcp_packet), 0, (struct sockaddr *)&(dhc->skt), sizeof dhc->skt)) < 0) {
					perror("sendto");
					exit(1);
			}
			//			fprintf(stderr, "send REQUEST\n");
			print_dhcp_packet(&packet, 1);
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
		  //fprintf(stderr, "Received packet. message:DHCPACK\n");
			print_dhcp_packet(dhc->buf, 0);
			uint16_t time = dhc->buf->time;
			uint32_t ip = dhc->buf->address;
			uint32_t mask = dhc->buf->netmask; 
			dhc->cli_addr.s_addr = ip;//network order
			dhc->netmask.s_addr = mask; //network order
			dhc->ttl = dhc->buf->time;
			dhc->ipttl = dhc->buf->time;
			dhc->ttlcounter = (dhc->ttl)/2;
			//dhc->ipsetor = 1;
			ip = ntohl(ip);
			mask = ntohl(mask);
			struct in_addr ipin;
			ipin.s_addr = ip;
			char * ipstring = inet_ntoa(ipin);
			fprintf(stderr, "\nIP set. IP:%s  ", ipstring);
			struct in_addr maskin;
			maskin.s_addr = mask;
			char * maskstring = inet_ntoa(maskin);
			fprintf(stderr, "Mask:%s\n", maskstring);
			//print_dhcp_packet(&packet, 1);
			status_change(dhc, STAT_IN_USE);
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
		  //fprintf(stderr, "Received packet. message:DHCPACK\n");
		  print_dhcp_packet(dhc->buf, 0);
			uint16_t time = dhc->buf->time;
			uint32_t ip = dhc->buf->address;
			uint32_t mask = dhc->buf->netmask; 
			dhc->cli_addr.s_addr = ip;//network order
			dhc->netmask.s_addr = mask; //network order
			dhc->ttl = dhc->buf->time;
			dhc->ttlcounter = (dhc->ttl)/2;
			dhc->ipsetor = 1;

			ip = ntohl(ip);
			mask = ntohl(mask);
			struct in_addr ipin;
			ipin.s_addr = ip;
			char * ipstring = inet_ntoa(ipin);
			fprintf(stderr, "Ip available time extended. IP: %s ", ipstring);
			struct in_addr maskin;
			maskin.s_addr = mask;
			char * maskstring = inet_ntoa(maskin);
			fprintf(stderr, "Mask:%s\n", maskstring);
			status_change(dhc, STAT_IN_USE);
			return 0;
		}
		
	} else {
		fprintf(stderr, "Message error: state is not true, should be DHCPACK\n");
		exit(1);
		return -1;
	}
}
static int set_signal_and_timer() {
	flag_alrm = 0;
	flag_hup = 0;
    signal(SIGALRM, signal_alrm);
    signal(SIGHUP, signal_hup);
	struct timeval interval = { 1, 0 };
    struct itimerval itimer = {interval, interval};
    int ret = setitimer(ITIMER_REAL, &itimer, NULL);
    return 0;
}
static void signal_alrm() {
	flag_alrm++;
	return ;
}
static void signal_hup() {
	flag_hup++;
	return;
}
static void signal_alrm_etc(struct dhcpc *dhc) {
				struct dhcp_packet packet;
	if(flag_hup != 0) {
		if (dhc->stat != STAT_IN_USE && dhc->stat != STAT_WAIT_EXT_ACK) {
			fprintf(stderr, "SIGHUP: IP is not in use.\n");
			exit(1);
		} else {
			fprintf(stderr, "SIGHUP: RELEASE IP adress and exit.\n");
			uint32_t ip = dhc->cli_addr.s_addr;
			init_dhcp_packet(&packet, DHCPRELEASE, 0, 0, ip, 0);
			int count;
			if ((count = sendto(dhc->s, &packet, sizeof(struct dhcp_packet), 0, (struct sockaddr *)&(dhc->skt), sizeof dhc->skt)) < 0) {
					perror("sendto");
					exit(1);
			}	
		}
		print_dhcp_packet(&packet, 1);
		exit(1);
		return;
	}
	if (flag_alrm != 0) {
		(dhc->ttlcounter)--;
		if (dhc->ttlcounter <= 0) alarm_work_dhcpc(dhc);
	}
	flag_alrm = 0;
	return;
}
static void alarm_work_dhcpc(struct dhcpc * dhc) {
	struct dhcp_packet packet;
	switch (dhc->stat) {
		case STAT_WAIT_OFFER:
			{
			uint16_t time = dhc->buf->time;
			uint32_t ip = htonl(dhc->cli_addr.s_addr);
			uint32_t mask = htonl(dhc->netmask.s_addr); 
			init_dhcp_packet(&packet, DHCPDISCOVER, CODE_IN_REQUEST_EXTEND, 0,0,0);
			int count;
			if ((count = sendto(dhc->s, &packet, sizeof(struct dhcp_packet), 0, (struct sockaddr *)&(dhc->skt), sizeof dhc->skt)) < 0) {
					perror("sendto");
					exit(1);
			}
			//			fprintf(stderr, "send REQUEST\n");
			dhc->ttlcounter = PACKET_WAIT_TTL;
			print_dhcp_packet(&packet, 1);
			status_change(dhc, STAT_WAIT_OFFER_2ND);
			}
		break;
		case STAT_WAIT_OFFER_2ND:
			fprintf(stderr, "TIMEOUT EXIT: STAT_WAIT_OFFER_2ND\n");
			exit(1);
		break;
		case STAT_WAIT_ACK:
			{
			uint16_t time = dhc->buf->time;
			uint32_t ip = htonl(dhc->cli_addr.s_addr);
			uint32_t mask = htonl(dhc->netmask.s_addr); 
			init_dhcp_packet(&packet, DHCPREQUEST, CODE_IN_REQUEST_FIRST, time, ip, mask);
			int count;
			if ((count = sendto(dhc->s, &packet, sizeof(struct dhcp_packet), 0, (struct sockaddr *)&(dhc->skt), sizeof dhc->skt)) < 0) {
					perror("sendto");
					exit(1);
			}
			dhc->ttlcounter = PACKET_WAIT_TTL;
			//			fprintf(stderr, "send REQUEST\n");
			print_dhcp_packet(&packet, 1);
			status_change(dhc, STAT_WAIT_ACK_2ND);
			}
		break;
		case STAT_WAIT_ACK_2ND:
			fprintf(stderr, "TIMEOUT EXIT: STAT_WAIT_ACK_2ND\n");
			exit(1);
		break;
		case STAT_IN_USE:
			{
			uint16_t time = dhc->ipttl;
			uint32_t ip = dhc->cli_addr.s_addr;
			uint32_t mask = dhc->netmask.s_addr;
			init_dhcp_packet(&packet, DHCPREQUEST, CODE_IN_REQUEST_EXTEND, time, ip, mask);
			int count;
			if ((count = sendto(dhc->s, &packet, sizeof(struct dhcp_packet), 0, (struct sockaddr *)&(dhc->skt), sizeof dhc->skt)) < 0) {
					perror("sendto");
					exit(1);
			}
			dhc->ttlcounter = dhc->ipttl/2;
			//			fprintf(stderr, "send REQUEST\n");
			print_dhcp_packet(&packet, 1);
			status_change(dhc, STAT_WAIT_ACK_2ND);
			}
		break;
		case STAT_WAIT_EXT_ACK:
			fprintf(stderr, "TIMEOUT EXIT: STAT_WAIT_EXT_ACK\n");
			exit(1);
		break;
		default:
			fprintf(stderr, "TIMEOUT but no stat\n");
		break;
	}
}
