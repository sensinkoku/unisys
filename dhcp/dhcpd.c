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
#include <errno.h>
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

static void signal_alrm();
static void signal_alrm_etc(struct dhcpd *dd);
static int alarm_work_dhcpd(struct dhcpd *dd, struct c_entry *c);

static int getipttl_from_file(struct dhcpd *dhc, char *filename);
///
static flag_alrm;


// define extern functions

void init_dhcpd(struct dhcpd * dd,int argc, char * argv[]) {
  set_signal_and_timer();
  init_ip_struct(&(dd->ip_list_head), 0, 0);
  init_head_struct(&(dd->c_entry_head), 0);
  int i;
  if ((i = mydhcpd_input_check(argc, argv)) < 0) {
  	exit(1);
  }
  getipttl_from_file(dd, argv[1]);
  if((i = init_ip_list_from_arg(&(dd->ip_list_head), argv[1])) < 0) {
  	exit(1);
  }
  print_ip_list(&(dd->ip_list_head));
  dd->buf = malloc (sizeof (struct dhcp_packet));
  socket_and_bind(dd); //open socket and bind
  //setting ip
  return;
}
void loop_dhcpd(struct dhcpd * dd) {
	for (;;) {
		int i;
		struct c_entry *client = NULL;
		// recv part
		if ((i =recv_packet(dd)) < 0) continue;
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
	//	bzero(&(dd->myskt), sizeof dd->myskt);
	in_port_t portnum = DHCPSERVERPORT;
	dd->myskt.sin_family = AF_INET;
	dd->myskt.sin_port = htons(portnum);
	//Ip setting relys on OS
	dd->myskt.sin_addr.s_addr = htonl(INADDR_ANY);
	//test
	struct in_addr test;
	test.s_addr = ntohl(dd->myskt.sin_addr.s_addr);
 //	char * stringip = inet_ntoa(test);
	//fprintf (stderr, "DHCP server  ip is: %s\n", inet_ntoa(test));
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
		case STAT_WAIT_REQUEST2:
			if (msg_request(dd, client) < 0) {
				fprintf(stderr, "not proper message in stat WAIT_REQUEST_2\n");
				return -1;
			}
			break;
		case STAT_IN_USE:
			if (msg_release(dd, client) < 0) {
				fprintf(stderr, "not proper message in stat STAT_IP_ASSIGNMENT\n");
				return -1;
			}
			break;
		default:
		  fprintf(stderr, "error: client have no status\n");
		  return -1;
			//error messaage
			break;
		}
	}
}

static int recv_packet(struct dhcpd * dd) {
		int fds;
		int count;
		fd_set rdfds;
		FD_ZERO(&rdfds);
		FD_SET(dd->s, &rdfds);
		/*
		if ((fds = select((dd->s)+1, &rdfds, NULL, NULL, NULL)) < 0) {
			fprintf(stderr, "select miss\n");
		} else if (rv == 0) {		// timeout
			fprintf(stderr, "[NO PACKET] Waiting...\r", MSG_TIMEOUT);
			return -1;
		} else {	// data recieved
		if (FD_ISSET(hpr->mysocd, &rdfds)) {*/
		fds = select((dd->s)+1, &rdfds, NULL, NULL, NULL);
		if (errno == 4) {
			//fprintf(stderr,"debug timeout\n");
			fprintf(stderr,"/sec");
			signal_alrm_etc(dd);
			errno = 0;
			return -1;
		} else {
		socklen_t sktlen;
		sktlen = sizeof dd->bufskt;
		if ((count  = recvfrom (dd->s, dd->buf, sizeof (struct dhcp_packet), 0, (struct sockaddr *)&(dd->bufskt), &sktlen)) < 0) {
			perror ("recvfrom error");
			exit(1);
		}
		print_dhcp_packet(dd->buf, 0, htonl(dd->bufskt.sin_addr.s_addr));
		return 0;
	}
}

//return 1 if not exist, return 0 already exist, if case 1 client pointer is NULL
static int client_check(struct dhcpd * dd, struct c_entry **client) {
  uint32_t id;
  id = ntohl(dd->bufskt.sin_addr.s_addr);
  search_client(&(dd->c_entry_head), client, id);
  if (*client == NULL) {
 //    client = make_new_client(&(dd->c_entry_head), dd->bufskt.sin_addr.s_addr, 0, 0, NOT_IP_ASSIGNED, PACKET_WAIT_TTL);
    //fprintf(stderr,"debug client is null\n");
    	return 1;
	} else {
		return 0;
	}
}

static int mydhcpd_input_check(int argc, char * argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: ./mydhcpd <fileneme>\n");
    return -1;
  }
  return 0;
}
static int msg_discover(struct dhcpd * dd, struct c_entry *client) {
	struct dhcp_packet packet;
	if (dd->buf->type == DHCPDISCOVER) {
	  //fprintf(stderr, "RECEIVE MESSAGE :DHCPDISCOVER\n");
				uint8_t code;
				struct ip_list * ip;
				int count;
				if((count = getrm_ip_from_list(&(dd->ip_list_head), &ip, dd->ipttl)) < 0) {
					fprintf(stderr, "msg_discover: can't assign because no ip\n");
					code = 1;
				} else{
					uint32_t n_ip;
					uint32_t n_mask;
					uint32_t id;
					id = ntohl(dd->bufskt.sin_addr.s_addr);
					n_ip = ntohl(ip->ip);
					n_mask = ntohl(ip->mask);
					client = make_new_client(&(dd->c_entry_head), id, n_ip, n_mask, STAT_WAIT_DISCOVER, PACKET_WAIT_TTL);
					code = 0;
				}
				uint32_t s_ip, s_mask;
				s_ip = htonl(ip->ip);
				s_mask = htonl(ip->mask);
				init_dhcp_packet(&packet, DHCPOFFER,code, dd->ipttl,s_ip, s_mask);
				socklen_t sklen = sizeof(dd->bufskt);
				sendto(dd->s, &packet, sizeof(struct dhcp_packet), 0, (struct sockaddr *)&(dd->bufskt), sklen);
				//fprintf(stderr, "SEND MESSAGE :DHCPOFFER\n");
				if (client == NULL) {
				  print_dhcp_packet(&packet, 1, ntohl(dd->bufskt.sin_addr.s_addr));
				}else {
				  print_dhcp_packet(&packet, 1, client->id.s_addr);
				}
				if (client != NULL) client_status_change(client, STAT_WAIT_REQUEST);
				return 0;
			} else {
				fprintf(stderr, "Message error: state is not true, should be DHCPDISCOVER\n");
				return -1;
			}
}
static int msg_request(struct dhcpd * dd, struct c_entry *client) {
	struct dhcp_packet packet;
	if (dd->buf->type == DHCPREQUEST) {
	  //	  fprintf(stderr, "RECEIVE MESSAGE :DHCPREQUEST\n");
	  uint8_t code;
		//send DHCPACK
		if (client->cli_addr.s_addr == dd->buf->address && client->netmask.s_addr == dd->buf->netmask && client->ttl <= dd->ipttl) {
		//search client by id? and not exist and can't match ip and mask
			code = 0; //success
		} else {
			fprintf(stderr, "In message request, illegal IP and Mask or TTL\n");
			code = 4; //miss
		}
		init_dhcp_packet(&packet, DHCPACK, code, dd->buf->time, client->cli_addr.s_addr, client->netmask.s_addr);
		socklen_t sklen = sizeof(dd->bufskt);
		sendto(dd->s, &packet, sizeof(struct dhcp_packet), 0, (struct sockaddr *)&(dd->bufskt), sklen);
		//fprintf(stderr, "SEND MESSAGE\n");
		print_dhcp_packet(&packet, 1, client->id.s_addr);
		client->ttl = dd->buf->time;
		client->ttlcounter = dd->buf->time;
		if (code == 0) client_status_change(client, STAT_IN_USE);// can't use stat_wait_request_2
		else rm_client(&(dd->ip_list_head), client);// remove no siyou
	} else {
		fprintf(stderr, "Message error: state is not true, should be DHCPOFFER\n");
		return -1;
	}
}
static int msg_release(struct dhcpd * dd, struct c_entry *client) {
	struct dhcp_packet packet;
	if (dd->buf->type == DHCPRELEASE) {
	  //	  	  fprintf(stderr, "RECEIVE MESSAGE :DHCPRELEASE\n");
		rm_client(&(dd->ip_list_head), client);
	} else if (dd->buf->type == DHCPREQUEST) {
		uint8_t code;
		//send DHCPACK
		if (client->cli_addr.s_addr == dd->buf->address && client->netmask.s_addr == dd->buf->netmask && client->ttl <= dd->ipttl) {
		//search client by id? and not exist and can't match ip and mask
			code = 0; //success
		} else {
			fprintf(stderr, "In message request, illegal IP and Mask or TTL\n");
			code = 4; //miss
		}
		init_dhcp_packet(&packet, DHCPACK, code, dd->buf->time, client->cli_addr.s_addr, client->netmask.s_addr);
		socklen_t sklen = sizeof(dd->bufskt);
		sendto(dd->s, &packet, sizeof(struct dhcp_packet), 0, (struct sockaddr *)&(dd->bufskt), sklen);
		//fprintf(stderr, "SEND MESSAGE\n");
		print_dhcp_packet(&packet, 1, client->id.s_addr);
		client->ttl = dd->buf->time;
		client->ttlcounter = dd->buf->time;
		if (code == 0) client_status_change(client, STAT_IN_USE);// can't use stat_wait_request_2
		else rm_client(&(dd->ip_list_head), client);
	}else{
		fprintf(stderr, "Message error: state is not true, should be STAT_IP_ASSIGNMENT\n");
		return -1;
	}
}
static int set_signal_and_timer() {
	flag_alrm = 0;
    signal(SIGALRM, signal_alrm);
	struct timeval interval = { 1, 0 };
    struct itimerval itimer = {interval, interval};
    int ret = setitimer(ITIMER_REAL, &itimer, NULL);
    return 0;
}
static void signal_alrm() {
	flag_alrm++;
	return ;
}
static void signal_alrm_etc(struct dhcpd *dd) {
	if(flag_alrm != 0) {
		struct c_entry * c;
		struct c_entry *nc;
		c = dd->c_entry_head.fp;
		while (c != &(dd->c_entry_head)) {
			c->ttlcounter = c->ttlcounter - flag_alrm;
			nc = c->fp;
			if (c->ttlcounter < 0) {
				alarm_work_dhcpd(dd, c);
			}
			c = nc;
		}
	}
	flag_alrm = 0;
	return;
}
static int alarm_work_dhcpd(struct dhcpd *dd, struct c_entry *c) {
	fprintf(stderr, "\n");
	switch(c->stat) {
		case STAT_WAIT_REQUEST:
			client_status_change(c, STAT_WAIT_REQUEST2);
			c->stat = STAT_WAIT_REQUEST2;
			c->ttlcounter = PACKET_WAIT_TTL;
			c->ttl = PACKET_WAIT_TTL;
			fprintf(stderr, "DBG Timeout REQUEST\n");
		break;
		case STAT_WAIT_REQUEST2:
			fprintf(stderr, "DBG Timeout REQUEST2\n");
			rm_client(&(dd->ip_list_head), c);
		break;
		case STAT_IN_USE:
			fprintf(stderr, "Timeout in STAT_IN_USE remove client\n");
			rm_client(&(dd->ip_list_head), c);
		break;
		default:
			fprintf(stderr, "debug: TTL < 0 but state is not \n");
		break;
	}
	return 0;
}
static int getipttl_from_file(struct dhcpd *dd, char *filename) {
  FILE * fp;
  char line[100];
  int i;
  uint32_t ip;
  uint32_t mask;
  if((fp = fopen(filename, "r")) == NULL) {
    fprintf (stderr, "IP list file not exist\n");
    exit(1);
    return -1;
  }
  if (fgets(line, 100, fp) == NULL) {
	fprintf (stderr, "File is empty\n");
    exit(1);
    return -1;
  }
  sscanf(line, "%d", &(dd->ipttl));
  fprintf(stderr, "ip ttl is %d\n", dd->ipttl);
  return 0;
}
