#include "dhcpd.h"
#include "centry.h"
#include "ip_list.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>

//declare static functions
static int socket_and_bind(struct dhcpd * dd);
static int work_dhcp_server(struct dhcpd * dd, struct c_entry *client);
static int recv_packet(struct dhcpd * dd);
static int client_check(struct dhcpd * dd, struct c_entry *client);
static int mydhcpd_input_check(int argc, char * argv[]);
// define extern functions
void init_dhcpd(struct dhcpd * dd,int argc, char * argv[]) {
  init_ip_struct(dd->ip_list_head, 0, 0);
  //  dd->ip_list_head.fp = dd->ip_list_head;
  //dd->ip_list_head.bp = dd->ip_list_head;
  init_centry_struct(0, 0);
  //dd->c_entry_head.fp = c_entry_head;
  //dd->c_entry_head.bp = c_entry_head;
  mydhcpd_input_check(argc, argv);
  init_ip_list_from_arg(dd->ip_list_head, argv[1]); //errorcheck
  //open socket and bind
  socket_and_bind(dd);
  //setting ip
}
void loop_dhcpd(struct dhcpd * dd) {
	for (;;) {
		struct c_entry *client;
		// recv part
		recv_packet(dd);
		//search client list by ip part
		client_check(dd, client);
		//work as dhcp
		work_dhcp_server(dd, client);
	}
}


// define static functions
static int socket_and_bind(struct dhcpd * dd) {
	if ((dd->s = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		perror ("server socket open error");
		exit(1);
	}
	bzero(&(dd->myskt), sizeof dd->myskt);
	in_port_t portnum = DHCPSERBERPORT;
	dd->myskt.sin_family = AF_INET;
	dd->myskt.sin_port = htons(portnum);
	//Ip setting relys on OS
	dd->myskt.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(dd->s, (struct sockaddr *) &(dd->myskt), sizeof dd->myskt) < 0) {
		perror("server bind error");
		exit(1);
	}
	return 0;
}

static int work_dhcp_server(struct dhcpd * dd, struct c_entry *client) {
		switch(client->status) {
		case STAT_WAIT_DISCOVER:
			if (client->msgtype == DHCPDISCOVER) {
				struct ip_list* ip = getrm_ip_from_list();
				//sendto(); make_dhcp_packet();
				client->status = STAT_WAIT_REQUEST;
			} else {
				//errors
			}
			break;
		case STAT_WAIT_REQUEST:
			if (client->msgtype == DHCPREQUEST) {
				//sendto(); make_dhcp_packet();

				//???? maybe STAT_IP_ASSIGNMENT
				client->status = STAT_WAIT_DISCOVER;
			} else {
				//errors
			}
			break;
		default:
			//error messaage
			break;
		}
}
static int recv_packet(struct dhcpd * dd) {
		int count;
		socklen_t sktlen;
		sktlen = sizeof dd->bufskt;
		if ((count  = recvfrom (dd->s, dd->buf, sizeof dd->buf, 0, dd->bufskt, &sktlen)) < 0) {
			perror ("recvfrom error");
			exit(1);
		}
		return 0;
}
static int client_check(struct dhcpd * dd, struct c_entry *client) {
	if (search_client(ntohl(dd->bufskt.sin_addr.s_addr)) == NULL) {
			client = make_new_client(dd->c_entry_head, dd->bufskt);
	} else {
			
	}
	return 0;
}
static int mydhcpd_input_check(int argc, char * argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: ./dhcpd <fileneme>\n");
    return -1;
  }
  return 0;
}




