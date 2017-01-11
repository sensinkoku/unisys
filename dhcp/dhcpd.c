#include "dhcpd.h"
#include "centry.h"
#include "ip_list.h"
#include "dhcp_packet.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <strings.h>
#include <stdio.h>
#include <arpa/inet.h>
//#include <errono>
#include <unistd.h>
#include <signal.h>

//declare static functions
static int socket_and_bind(struct dhcpd * dd);
static int work_dhcp_server(struct dhcpd * dd, struct c_entry *client);
static int recv_packet(struct dhcpd * dd);
static int client_check(struct dhcpd * dd, struct c_entry **client);
static int mydhcpd_input_check(int argc, char * argv[]);
static int set_signal_and_timer ();

static int msg_discover (struct dhcpd * dd, struct c_entry *client);
static int msg_request (struct dhcpd * dd, struct c_entry *client);
static int msg_release (struct dhcpd * dd, struct c_entry *client);
// define extern functions
void init_dhcpd(struct dhcpd * dd,int argc, char * argv[]) {
  set_signal_and_timer();
  init_ip_struct(&(dd->ip_list_head), 0, 0);
  init_head_struct(&(dd->c_entry_head), 0);
  int i;
  if ((i = mydhcpd_input_check(argc, argv)) < 0) {
  	exit(1);
  }
  if((i = init_ip_list_from_arg(&(dd->ip_list_head), argv[1])) < 0) {
  	exit(1);
  }
  dd->buf = malloc (sizeof (struct dhcp_packet));
  socket_and_bind(dd); //open socket and bind
  //setting ip
  return;
}
void loop_dhcpd(struct dhcpd * dd) {
	for (;;) {
		struct c_entry *client = NULL;
		// recv part
		recv_packet(dd);
		//search client list by ip part
		client_check(dd, &client); // not necessary remove
		//work as dhcp
		work_dhcp_server(dd, client);
	}
}


// define static functions
static int socket_and_bind(struct dhcpd * dd) {
	if ((dd->s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror ("server socket open error");
		exit(1);
	}
	bzero(&(dd->myskt), sizeof dd->myskt);
	in_port_t portnum = DHCPSERVERPORT;
	dd->myskt.sin_family = AF_INET;
	dd->myskt.sin_port = htons(portnum);
	//Ip setting relys on OS
	dd->myskt.sin_addr.s_addr = htonl(INADDR_ANY);
	//test
	struct in_addr test;
	test.s_addr = ntohl(dd->myskt.sin_addr.s_addr);
//	char * stringip = inet_ntoa(test);
	fprintf (stderr, "DHCP server  ip is: %s\n", inet_ntoa(test));
	//test
	int count;
	if ((count = bind(dd->s, (struct sockaddr *)&(dd->myskt), sizeof dd->myskt)) < 0) {
		perror("server bind error");
		exit(1);
	}
	return 0;
}

static int work_dhcp_server(struct dhcpd * dd, struct c_entry *client) {
	struct dhcp_packet packet;
		if (client == NULL) {
			if (dd->buf->type == DHCPDISCOVER) {
				msg_discover(dd, client);
				return 0;
			} else {
				fprintf(stderr, "not proper message in stat WAIT_DISCOVER\n");
				return -1;
			}
		} else {
		switch(client->stat) {
		/*case STAT_WAIT_DISCOVER:
			if (msg_discover(dd, client) < 0) {
				//error;
			}
			break;*/
		case STAT_WAIT_REQUEST:
			if (msg_request(dd, client) < 0) {
				fprintf(stderr, "not proper message in stat WAIT_REQUEST\n");
				return -1;
			}
			break;
		case STAT_WAIT_REQUEST_2:
			if (msg_request(dd, client) < 0) {
				fprintf(stderr, "not proper message in stat WAIT_REQUEST_2\n");
				return -1;
			}
			break;
		case STAT_IP_ASSIGNMENT:
			if (msg_release(dd, client) < 0) {
				fprintf(stderr, "not proper message in stat STAT_IP_ASSIGNMENT\n");
				return -1;
			}
		default:
		  fprintf(stderr, "error: client have no status\n");
		  return -1;
			//error messaage
			break;
		}
	}
}

static int recv_packet(struct dhcpd * dd) {
		int count;
		socklen_t sktlen;
		sktlen = sizeof dd->bufskt;
		if ((count  = recvfrom (dd->s, dd->buf, sizeof (struct dhcp_packet), 0, (struct sockaddr *)&(dd->bufskt), &sktlen)) < 0) {
			perror ("recvfrom error");
			exit(1);
		}
		print_dhcp_packet(dd->buf);
		return 0;
}

//return 1 if not exist, return 0 already exist, if case 1 client pointer is NULL
static int client_check(struct dhcpd * dd, struct c_entry **client) {
  uint32_t id;
  id = dd->bufskt.sin_addr.s_addr;
  search_client(&(dd->c_entry_head), client, id);
  if (client == NULL) {
//    client = make_new_client(&(dd->c_entry_head), dd->bufskt.sin_addr.s_addr, 0, 0, NOT_IP_ASSIGNED, PACKET_WAIT_TTL);
    fprintf(stderr,"debug client is null\n");
    	return 1;
	} else {
		return 0;
	}
}

static int mydhcpd_input_check(int argc, char * argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: ./dhcpd <fileneme>\n");
    return -1;
  }
  return 0;
}
static int msg_discover(struct dhcpd * dd, struct c_entry *client) {
	struct dhcp_packet packet;
	if (dd->buf->type == DHCPDISCOVER) {
	  fprintf(stderr, "RECEIVE MESSAGE :DHCPDISCOVER\n");
				uint8_t code;
				struct ip_list * ip;
				if((ip = getrm_ip_from_list(&(dd->ip_list_head))) == NULL) {
					fprintf(stderr, "msg_discover: can't assign because no ip\n");
					code = 129;
				} else{
					client = make_new_client(&(dd->c_entry_head), dd->bufskt.sin_addr.s_addr, ip->ip, ip->mask, STAT_WAIT_DISCOVER, PACKET_WAIT_TTL);
					code = 0;
				}
				uint32_t s_ip, s_mask;
				s_ip = htonl(ip->ip);
				s_mask = htonl(ip->mask);
				init_dhcp_packet(&packet, DHCPOFFER,code,REQUEST_WAIT_TIME,s_ip, s_mask);
				socklen_t sklen = sizeof(dd->bufskt);
				sendto(dd->s, &packet, sizeof(struct dhcp_packet), 0, (struct sockaddr *)&(dd->bufskt), sklen);
				fprintf(stderr, "SEND MESSAGE :DHCPOFFER\n");
				print_dhcp_packet(&packet);
				client_status_change(client, STAT_WAIT_REQUEST);
				return 0;
			} else {
				fprintf(stderr, "Message error: state is not true, should be DHCPDISCOVER\n");
				return -1;
			}
}
static int msg_request(struct dhcpd * dd, struct c_entry *client) {
	struct dhcp_packet packet;
	if (dd->buf->type == DHCPREQUEST) {
	  fprintf(stderr, "RECEIVE MESSAGE :DHCPREQUEST\n");
	  uint8_t code;
		//send DHCPACK
		if (client->cli_addr.s_addr == dd->buf->address && client->netmask.s_addr == dd->buf->netmask && client->ttl >= dd->buf->time) {
		//search client by id? and not exist and can't match ip and mask
			code = 0; //success
		} else {
			fprintf(stderr, "In message request, illegal IP and Mask or TTL\n");
			code = 4; //miss
		}
		init_dhcp_packet(&packet, DHCPACK, code, IP_TTL, client->cli_addr.s_addr, client->netmask.s_addr);
		socklen_t sklen = sizeof(dd->bufskt);
		sendto(dd->s, &packet, sizeof(struct dhcp_packet), 0, (struct sockaddr *)&(dd->bufskt), sklen);
		fprintf(stderr, "SEND MESSAGE :DHCPACK\n");
		print_dhcp_packet(&packet);
		if (code == 0) client_status_change(client, STAT_IP_ASSIGNMENT);// can't use stat_wait_request_2
		else rm_client(client);// remove no siyou
	} else {
		fprintf(stderr, "Message error: state is not true, should be DHCPOFFER\n");
		return -1;
	}
}
static int msg_release(struct dhcpd * dd, struct c_entry *client) {
	if (dd->buf->type == STAT_IP_ASSIGNMENT) {
	  	  fprintf(stderr, "RECEIVE MESSAGE :DHCPRELEASE\n");
		rm_client(client);
	} else {
		fprintf(stderr, "Message error: state is not true, should be STAT_IP_ASSIGNMENT\n");
		return -1;
	}
}
static int set_signal_and_timer() {
	struct itimerval timer;
	timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 1000000;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 1000000;
	//sigaction(SIGALRM, search_ttl_and_decrease_time);
 	//if (setitimer(ITIMER_REAL, &timer, NULL) < 0) {
 	//	perror("setitimer error");
 	//	exit(1);
 	//}
}

