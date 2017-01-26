#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define DATASIZE 1024
#define FTPPORTNUM 50021
#define PATHLENGTH 1024

#define STAT_TCP_CONNECT 0
#define STAT_RECVMSG 1

#define STAT_INITIAL 0
#define STAT_WAIT_COMMAND 1
#define STAT_WAIT_PWD 2
#define STAT_WAIT_CWD 3
#define STAT_WAIT_LIST_OK 4
#define STAT_WAIT_LIST_DATA 5
#define STAT_WAIT_RETR_OK 6
#define STAT_WAIT_RETR_DATA 7
#define STAT_WAIT_STOR_OK 8

#define FTPMSG_QUIT 0x01
#define FTPMSG_PWD 0x02
#define FTPMSG_CWD 0x03
#define FTPMSG_LIST 0x04
#define FTPMSG_RETR 0x05
#define FTPMSG_STOR 0x06
#define FTPMSG_OK 0x10
#define FTPMSG_CMD_ERR 0x11
#define FTPMSG_FILE_ERR 0x12
#define FTPMSG_UNKWN_ERR 0x13
#define FTPMSG_DATA 0x20

#define CODE_OK 0x00
#define CODE_OK_DATA_STOC 0x01
#define CODE_OK_DATA_CTOS 0x02
#define CODE_CMDERR_GRAMMER 0x01
#define CODE_CMDERR_UNDEFCOM 0x02
#define CODE_CMDERR_PROTERR 0x03
#define CODE_FILEERR_NODIR 0x00
#define CODE_FILEERR_NOACCESS 0x01
#define CODE_UNKWNERR 0x05
#define CODE_DATA_NOTLAST 0x01
#define CODE_DATA_LAST 0x00



struct ftpdata{
	uint8_t type;
	uint8_t code;
	uint16_t length;
	char data[DATASIZE];
};
struct ftphead {
	uint8_t type;
	uint8_t code;
	uint16_t length;	
};

struct ftpd {
	int s;
	int clis;
	int status;
	int pathlength;
	char * path;
	FILE * fp;	
	// below network order 

	struct addrinfo *res;
	struct sockaddr_in myskt;
	struct sockaddr_in cliskt;
	struct ftpdata *fdbuf;
	struct ftphead *fdhbuf;
};


struct command_table{
  char * cmd;
  void (*func)(struct ftpd *, int, char *[]);
};

static int init_ftpd(struct ftpd * ftpd, int argc, char *argv[]);
static int loop_ftpd(struct ftpd * ftpd);

static int loadargs(struct ftpd * ftpd, int argc, char *argv[]);
static int init_ftpd_struct(struct ftpd * ftpd);
static int setsig(struct ftpd * ftpd);
static int tcpstart(struct ftpd * ftpd);
//static int make_connection(struct ftpd * ftpd);
static int command_work(struct ftpd * ftpd);
static int ftp_work(struct ftpd * ftpd);

static int work_in_stat_connect(struct ftpd * ftpd);
static int work_in_recvmsg(struct ftpd * ftpd);

static int work_in_pwd(struct ftpd * ftpd);
static int work_in_cwd(struct ftpd * ftpd);
static int work_in_list_ok(struct ftpd * ftpd);
static int work_in_list_data(struct ftpd * ftpd);
static int work_in_retr_ok(struct ftpd * ftpd);
static int work_in_retr_data(struct ftpd * ftpd);
static int work_in_stor_ok(struct ftpd * ftpd);




static int msg_quit(struct ftpd * ftpd);
static int msg_pwd(struct ftpd * ftpd);
static int msg_cwd(struct ftpd * ftpd);
static int msg_list(struct ftpd * ftpd);
static int msg_retr(struct ftpd * ftpd);
static int msg_stor(struct ftpd * ftpd);
static int msg_ok(struct ftpd * ftpd);
static int msg_cmderr(struct ftpd * ftpd);
static int msg_fileerr(struct ftpd * ftpd);
static int msg_unkwnerr(struct ftpd * ftpd);
static int msg_data(struct ftpd * ftpd);
static int msg_data_retr(struct ftpd * ftpd);
static int msg_data_list (struct ftpd * ftpd);

// below client function message
static int msg_ok_cwd(struct ftpd * ftpd);
static int msg_ok_pwd(struct ftpd * ftpd);
static int msg_ok_stor(struct ftpd * ftpd);
static int msg_ok_retr(struct ftpd * ftpd);
static int msg_ok_list(struct ftpd * ftpd);

static int send_ftph(struct ftpd * ftpd, uint8_t type, uint8_t code, uint16_t datalength);
static int send_ftpmsg(struct ftpd * ftpd, uint8_t type, uint8_t code, char *data, uint16_t datalength);
static int send_ftpdata(struct ftpd * ftpd, char *data, uint16_t datalength);

static int getfilesize(FILE * fp);
static int client_status_change(struct ftpd * ftpd, int status);
static int server_status_change(struct ftpd * ftpd, int status){}
static void myinput(int* ac, char ** av, char * input);

static void quit_proc(struct ftpd * ftpd, int argc, char *argv[]);
static void pwd_proc(struct ftpd * ftpd, int argc, char *argv[]);
static void cd_proc(struct ftpd * ftpd, int argc, char *argv[]);
static void dir_proc(struct ftpd * ftpd, int argc, char *argv[]);
static void lpwd_proc(struct ftpd * ftpd, int argc, char *argv[]);
static void lcd_proc(struct ftpd * ftpd, int argc, char *argv[]);
static void ldir_proc(struct ftpd * ftpd, int argc, char *argv[]);
static void get_proc(struct ftpd * ftpd, int argc, char *argv[]);
static void put_proc(struct ftpd * ftpd, int argc, char *argv[]);
static void help_proc(struct ftpd * ftpd, int argc, char *argv[]);

static void print_packet(struct ftphead * packet);
static void print_received_packet(struct ftpd * ftpd);


static int init_ftpd(struct ftpd * ftpd, int argc, char *argv[]) {
	loadargs(ftpd, argc, argv);
	init_ftpd_struct(ftpd);
	setsig(ftpd);
	tcpstart(ftpd);
	return 0;
}
static int loop_ftpd(struct ftpd * ftpd) {
	for (;;) {
			if (ftpd->status == STAT_WAIT_COMMAND) {
				command_work(ftpd);
				continue;
			}
			ftp_work(ftpd);
	}
	return 0;
}

static int loadargs(struct ftpd * ftpd, int argc, char *argv[]) {
	if (argc == 1) {
		fprintf(stderr, "Error no argument. Usage: ./myftpc <server hostname>\n");
		exit(1);
	}
	if (argc >= 3) {
		fprintf(stderr, "Error too many arguments. Usage: ./myftpc <server hostname>\n");
		exit(1);
	}
	struct addrinfo hints;
	int err;
	int hostlength = strlen(argv[1]);
	char hostname[hostlength + 1];
	strncpy(hostname, argv[1], hostlength);
	hostname[hostlength] = '\0';
	char *serv = "50021\0";

	memset (&hints, 0, sizeof (hints));
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = PF_INET;
	//ftpd->res = (struct addrinfo *) malloc(sizeof(struct addrinfo));
	if ((err = getaddrinfo(hostname, serv, &hints, &(ftpd->res))) < 0) {
		fprintf(stderr, " Error getaddrinfo: %s\n", gai_strerror(err));
		exit(1);
	}
	return 0;
}


static int init_ftpd_struct(struct ftpd * ftpd) {
	ftpd->status = STAT_INITIAL;
	ftpd->fdbuf = (struct ftpdata * )malloc(sizeof(struct ftpdata));
	ftpd->fdhbuf = (struct ftphead * )malloc(sizeof(struct ftphead));
	return 0;
}

static int setsig(struct ftpd * ftpd) {
	return 0;
}

/*static int tcpstart(struct ftpd * ftpd) {
	if ((ftpd->s = socket(ftpd->res->ai_family, ftpd->res->ai_socktype, ftpd->res->ai_protocol)) < 0) {
		perror("socket error");
		exit(1);
	}
	in_port_t portnum = FTPPORTNUM;
	ftpd->myskt.sin_family = AF_INET;
	ftpd->myskt.sin_port = htons(portnum);
	ftpd->myskt.sin_addr.s_addr = htonl(INADDR_ANY);
	if (connect(ftpd->s, ftpd->res->ai_addr, ftpd->res->ai_addrlen) < 0) {
		perror("connect error");
		exit(1);
	}
	ftpd->status = STAT_WAIT_COMMAND;
	freeaddrinfo(ftpd->res);
	return 0;
}*/
/*static int tcpstart(struct ftpd * ftpd) {
	if ((ftpd->s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket error");
		exit(1);
	}
	in_port_t portnum = FTPPORTNUM;
	ftpd->myskt.sin_family = AF_INET;
	ftpd->myskt.sin_port = htons(portnum);
	struct in_addr ips;
	char * ipadress = "127.0.0.1";
	inet_aton(ipadress, &ips);
	ftpd->myskt.sin_addr.s_addr =ips.s_addr;
	size_t sizeskt = sizeof(ftpd->myskt);
	if (connect(ftpd->s, (struct sockaddr *)&(ftpd->myskt), sizeskt) < 0) {
		perror("connect error");
		exit(1);
	}
	ftpd->status = STAT_WAIT_COMMAND;
	freeaddrinfo(ftpd->res);
	return 0;
}*/
static int tcpstart(struct ftpd * ftpd) {
	if ((ftpd->s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket error");
		exit(1);
	}
	in_port_t portnum = FTPPORTNUM;
	ftpd->myskt.sin_family = AF_INET;
	ftpd->myskt.sin_port = htons(portnum);
	struct in_addr ips;
	//char * ipadress = "127.0.0.1";
	//inet_aton(ipadress, &ips);
	//ftpd->myskt.sin_addr.s_addr =ips.s_addr;
	//size_t sizeskt = sizeof(ftpd->myskt);
	if (connect(ftpd->s, ftpd->res->ai_addr, ftpd->res->ai_addrlen) < 0) {
		perror("connect error");
		exit(1);
	}
	ftpd->status = STAT_WAIT_COMMAND;
	freeaddrinfo(ftpd->res);
	return 0;
}

/*static int make_connection(struct ftpd * ftpd) {
		socklen_t sktlen;
		sktlen = sizeof ftpd->cliskt;
		if ((ftpd->clis = accept(ftpd->s, (struct sockaddr *)&(ftpd->cliskt), &sktlen) < 0) < 0) {
			perror("accept error");
			exit(1);
		}
		ftpd->status = STAT_WAIT_COMMAND;
		return 0;
}*/

static int command_work(struct ftpd * ftpd) {
	struct command_table cmd_tbl[] = {
  	{"quit", quit_proc},
  	{"pwd", pwd_proc},
  	{"cd", cd_proc},
  	{"dir", dir_proc},
  	{"lpwd", lpwd_proc},
  	{"lcd", lcd_proc},
  	{"ldir", ldir_proc},
  	{"get", get_proc},
  	{"put", put_proc},
  	{"help", help_proc},
  	{NULL, NULL}
	};
	int ac;
	char *av[16];
	char *input;
	input = (char *) malloc(512);
	struct command_table *p;
	fprintf(stderr, "myFTP%% ");
    myinput(&ac, av, input);
    for (p = cmd_tbl; p->cmd; p++) {
      if (strcmp(av[0], p->cmd) == 0) {
        (*p->func)(ftpd, ac, av);
	       break;
      }
    }
    if (p->cmd == NULL) fprintf(stderr, "unknown command: %s\n", av[0]);
	return 0;
}

static int ftp_work(struct ftpd * ftpd) {
		int count;
		if ((count = recv(ftpd->s, ftpd->fdbuf, sizeof (struct ftpdata), 0)) < 0) {
			perror("recvfrom error");
			exit(1);
		} else if (count == 0) {
			fprintf(stderr, "TCP Connection finished. Program exit.\n");
			exit(1);
		}
		print_received_packet(ftpd);
		switch(ftpd->status){
			case STAT_WAIT_PWD:
				work_in_pwd(ftpd);
			break;
			case STAT_WAIT_CWD:
				work_in_cwd(ftpd);
			break;
			case STAT_WAIT_LIST_OK:
				work_in_list_ok(ftpd);
			break;
			case STAT_WAIT_LIST_DATA:
				work_in_list_data(ftpd);
			break;
			case STAT_WAIT_RETR_OK:
				work_in_retr_ok(ftpd);
			break;
			case STAT_WAIT_RETR_DATA:
				work_in_retr_data(ftpd);
			break;
			case STAT_WAIT_STOR_OK:
				work_in_stor_ok(ftpd);
			break;
			default:
				fprintf(stderr, "NOT PROPER STATUS TO THIS MESSAGE\n");
			break;
		}
	return 0;
}

static int work_in_stat_connect(struct ftpd * ftpd) {
	switch (ftpd->fdbuf->type) {
		case FTPMSG_PWD: {
			msg_pwd(ftpd);
		}
		break;
		case FTPMSG_CWD: {
			msg_cwd(ftpd);
		}
		break;
		case FTPMSG_LIST: {
			msg_list(ftpd);
		}
		break;
		case FTPMSG_RETR: {
			msg_retr(ftpd);
		}
		break;
		case FTPMSG_STOR: {
			msg_stor(ftpd);
		}
		break;
		case FTPMSG_QUIT: {
			msg_quit(ftpd);
		}
		break;
		default: {
			fprintf(stderr, "Not proper Message in STAT_TCP_CONNECT\n");
			return -1;
		}
		break;
	}
	return 0;
}
static int work_in_recvmsg(struct ftpd * ftpd) {
	switch (ftpd->fdbuf->type) {
		case FTPMSG_DATA: {
			msg_data(ftpd);
		}
		break;
		default:{
			fprintf(stderr, "Not proper Message in STAT_RECVMSG\n");
			client_status_change(ftpd,STAT_WAIT_COMMAND);
			return -1;
		}
		break;
	}
	return 0;
}

static int work_in_pwd(struct ftpd * ftpd){
	switch (ftpd->fdbuf->type) {
		case FTPMSG_OK:
			msg_ok_pwd(ftpd);
		break;
		default:{
			fprintf(stderr, "Not proper Message in STAT_WAIT_PWD\n");
			client_status_change(ftpd,STAT_WAIT_COMMAND);
			return -1;
		}
		break;
	}
	return 0;
}
static int work_in_cwd(struct ftpd * ftpd){
	switch (ftpd->fdbuf->type) {
		case FTPMSG_OK:
			msg_ok_cwd(ftpd);
		break;
		default:{
			fprintf(stderr, "Not proper Message in STAT_WAIT_CWD\n");
			fprintf(stderr, "Illegal access for directory.\n");
			client_status_change(ftpd,STAT_WAIT_COMMAND);
			return -1;
		}
		break;
	}
	return 0;
}
static int work_in_list_ok(struct ftpd * ftpd){
		switch (ftpd->fdbuf->type) {
		case FTPMSG_OK:
			msg_ok_list(ftpd);
		break;
		default:{
			fprintf(stderr, "Not proper Message in STAT_WAIT_LIST_OK\n");
			fprintf(stderr, "No such file or directory\n");
			client_status_change(ftpd,STAT_WAIT_COMMAND);
			return -1;
		}
		break;
	}
	return 0;
}
static int work_in_list_data(struct ftpd * ftpd){
	switch (ftpd->fdbuf->type) {
			case FTPMSG_DATA:
				msg_data_list(ftpd);
			break;
			default:{
			fprintf(stderr, "Not proper Message in STAT_WAIT_LIST_DATA\n");
			client_status_change(ftpd,STAT_WAIT_COMMAND);
				return -1;
			}
		break;
		}
		return 0;

}
static int work_in_retr_ok(struct ftpd * ftpd){
		switch (ftpd->fdbuf->type) {
		case FTPMSG_OK:
			msg_ok_retr(ftpd);
		break;
		default:{
			fprintf(stderr, "Not proper Message in STAT_WAIT_RETR_OK\n");
			client_status_change(ftpd,STAT_WAIT_COMMAND);
			return -1;
		}
		break;
	}
	return 0;
}
static int work_in_retr_data(struct ftpd * ftpd){
	switch (ftpd->fdbuf->type) {
			case FTPMSG_DATA:
				msg_data_retr(ftpd);
			break;
			default:{
			fprintf(stderr, "Not proper Message in STAT_WAIT_RETR_DATA\n");
			client_status_change(ftpd,STAT_WAIT_COMMAND);
				return -1;
			}
		break;
		}
		return 0;
}
static int work_in_stor_ok(struct ftpd * ftpd){
		switch (ftpd->fdbuf->type) {
			case FTPMSG_OK:
			msg_ok_stor(ftpd);
			break;
			default:{
			fprintf(stderr, "Not proper Message in STAT_WAIT_STOR_OK\n");
			client_status_change(ftpd,STAT_WAIT_COMMAND);
				return -1;
			}
		break;
		}
		return 0;
}

static int msg_quit(struct ftpd * ftpd){
	fprintf(stderr, "QUIT MSG: QUIT TCP CONNECTION\n");
	close(ftpd->s);
	exit(1);
}
static int msg_pwd(struct ftpd * ftpd) {
	if (ftpd->status != STAT_TCP_CONNECT) {
		fprintf (stderr, "NOT PROPER MESSAGE PWD IN THIS STATUS.\n");
		exit(1);
	}
	send_ftpmsg(ftpd, FTPMSG_OK, CODE_OK, ftpd->path, (uint16_t)ftpd->pathlength);
	return 0;
}
static int msg_cwd(struct ftpd * ftpd) {
	if (ftpd->status != STAT_TCP_CONNECT) {
		fprintf (stderr, "NOT PROPER MESSAGE CWD IN THIS STATUS.\n");
		exit(1);
	}
	int pathlen = ftpd->fdbuf->length;
	char path [pathlen];
	//memset(path, '\0', pathlen+1);
	strncpy(path, ftpd->fdbuf->data, pathlen);
	if (chdir(path) < 0) {
		fprintf(stderr, "Change directory error, Illegal path.\n");
		send_ftph(ftpd, FTPMSG_FILE_ERR, CODE_FILEERR_NODIR,0);

	} else { //set path
		free(ftpd->path);
		ftpd->pathlength = pathlen;
		ftpd->path = (char *)malloc ((pathlen+1) * sizeof(char));
		memset(ftpd->path, '\0', ftpd->pathlength+1);
		strncpy(ftpd->path,path,pathlen);
		fprintf(stderr,"Chdir to %s", ftpd->path);
		send_ftph(ftpd, FTPMSG_OK, CODE_OK, 0);
	}
	return 0;
}
static int msg_list(struct ftpd * ftpd) {
	int pathlen = ftpd->fdbuf->length;
	FILE *fp;
	if (pathlen == 0) {
		if ((fp = popen("ls -l", "r")) == NULL) {
			fprintf(stderr, "ls -l error\n");
			send_ftph(ftpd, FTPMSG_FILE_ERR, CODE_FILEERR_NODIR, 0);
			return -1;
		}
	} else {
		char path [pathlen+1];
		memset(path, '\0', pathlen+1);
		strncpy(path, ftpd->fdbuf->data, pathlen);
		char ls[32+pathlen];
		char lsbuf[8] = "ls -l\0";
		strncpy (ls, lsbuf, sizeof lsbuf);
		strncat (ls, path, pathlen+1);
		if ((fp = popen(ls , "r")) == NULL) {
			fprintf(stderr, "ls -l error\n");
			send_ftph(ftpd, FTPMSG_FILE_ERR, CODE_FILEERR_NODIR, 0);
			return -1;
		}
	}
	send_ftph(ftpd, FTPMSG_OK, CODE_OK_DATA_STOC, 0);
	int filesize = getfilesize(fp);
	char data[filesize];
	fread(data, sizeof(char), filesize, fp);
	send_ftpdata(ftpd, data, filesize);
	fclose(fp);
	return 0;
}
static int msg_retr(struct ftpd * ftpd) {
	int pathlen = ftpd->fdbuf->length;
	char path [pathlen];
	//memset(path, '\0', pathlen+1);
	strncpy(path, ftpd->fdbuf->data, pathlen);
	FILE *fp;
	if ((fp = fopen(path, "rb")) == NULL) {
		fprintf(stderr, "no such file in server\n");
		send_ftph(ftpd,FTPMSG_FILE_ERR,CODE_FILEERR_NOACCESS,0);
		return -1;
	}
	send_ftph(ftpd, FTPMSG_OK, CODE_OK_DATA_STOC, 0);
	int filesize = getfilesize(fp);
	char data[filesize];
	fread(data, sizeof (char), filesize, fp);
	send_ftpdata(ftpd, data, filesize);
	fclose(fp);
	return 0;
}

static int msg_stor(struct ftpd * ftpd) {
	int pathlen = ftpd->fdbuf->length;
	char path [pathlen];
	//memset(path, '\0', pathlen+1);
	strncpy(path, ftpd->fdbuf->data, pathlen);
	if ((ftpd->fp = fopen(path, "wb")) == NULL) {
		fprintf(stderr, "Failed file access.\n");
		send_ftph(ftpd,FTPMSG_FILE_ERR,CODE_FILEERR_NOACCESS,0);
		return -1;
	}
	server_status_change(ftpd, STAT_RECVMSG);
	send_ftph(ftpd, FTPMSG_OK, CODE_OK_DATA_CTOS, 0);
	return 0;
}

static int msg_ok(struct ftpd * ftpd) {

}
static int msg_cmderr(struct ftpd * ftpd) {

}
static int msg_fileerr(struct ftpd * ftpd){}
static int msg_unkwnerr(struct ftpd * ftpd){}
static int msg_data(struct ftpd * ftpd) {
	fwrite(ftpd->fdbuf->data, sizeof (char), ftpd->fdbuf->length, ftpd->fp);
	if (ftpd->fdbuf->code == CODE_DATA_LAST) {
		server_status_change(ftpd, STAT_TCP_CONNECT);
		fclose(ftpd->fp);
	}
	return 0;
}
static int msg_ok_cwd(struct ftpd * ftpd){
	if (ftpd->fdbuf->code == CODE_OK) {
		fprintf(stderr,"Server directory changed.\n");
		client_status_change(ftpd, STAT_WAIT_COMMAND);
		return 0;
	} else {
		fprintf(stderr,"In MSG_OK code should be 0x00 in wait stat cwd.\n");
		exit(1);
	}
	return 0;
}
static int msg_ok_pwd(struct ftpd * ftpd){
	if (ftpd->fdbuf->code == CODE_OK) {
		char path[ftpd->fdbuf->length + 1];
		strncpy(path, ftpd->fdbuf->data, ftpd->fdbuf->length);
		path[ftpd->fdbuf->length] = '\0';
		fprintf(stdout,"Server current directory got. Path: %s\n", path);
		client_status_change(ftpd, STAT_WAIT_COMMAND);
		return 0;
	} else {
		fprintf(stderr,"In MSG_OK code should be 0x00 in wait stat pwd.\n");
		exit(1);
	}
	return 0;
}
static int msg_ok_stor(struct ftpd * ftpd){
	if (ftpd->fdbuf->code == CODE_OK_DATA_CTOS) {
		fprintf(stderr,"Server ready for put is ok.\n");
		int filesize = getfilesize(ftpd->fp);
		char data[filesize];
		fread(data, sizeof(char), filesize, ftpd->fp);
		//send datas
		//fprintf(stderr, "debug. filesize = %d data = %s", filesize, data);
		send_ftpdata(ftpd, data, filesize);
		fclose(ftpd->fp);
		client_status_change(ftpd, STAT_WAIT_COMMAND);
		return 0;
	} else {
		fprintf(stderr,"In MSG_OK code should be 0x01 in wait stat_wait_list.\n");
		exit(1);
	}
	return 0;
}
static int msg_ok_retr(struct ftpd * ftpd){
	if (ftpd->fdbuf->code == CODE_OK_DATA_STOC) {
		fprintf(stderr,"Server return result of get.\n");
		client_status_change(ftpd, STAT_WAIT_RETR_DATA);
		return 0;
	} else {
		fprintf(stderr,"In MSG_OK code should be 0x01 in wait stat_wait_list.\n");
		exit(1);
	}
	return 0;
}
static int msg_ok_list(struct ftpd * ftpd){
	if (ftpd->fdbuf->code == CODE_OK_DATA_STOC) {
		fprintf(stderr,"Server return result of dir.\n");
		client_status_change(ftpd, STAT_WAIT_LIST_DATA);
		return 0;
	} else {
		fprintf(stderr,"In MSG_OK code should be 0x01 in wait stat_wait_list.\n");
		exit(1);
	}
	return 0;
}
static int msg_data_retr(struct ftpd * ftpd){
	if (ftpd->fdbuf->code == CODE_DATA_NOTLAST) {
		fprintf(stderr,"Data comes (not end).\n");
		fwrite(ftpd->fdbuf->data, sizeof(char), ftpd->fdbuf->length, ftpd->fp);
		return 0;
	} else if (ftpd->fdbuf->code == CODE_DATA_LAST) {
		fprintf (stderr, "Data last segment.\n");
		fwrite(ftpd->fdbuf->data, sizeof(char), ftpd->fdbuf->length, ftpd->fp);
		fclose(ftpd->fp);
		client_status_change(ftpd, STAT_WAIT_COMMAND);
		return 0;
	} else {
		fprintf(stderr,"Error Illegal Message code in stat_data_wait.\n");
		exit(1);
	}
	return 0;
}
static int msg_data_list (struct ftpd * ftpd){
	if (ftpd->fdbuf->code == CODE_DATA_NOTLAST) {
		fprintf(stdout, "%s",ftpd->fdbuf->data);
		return 0;
	} else if (ftpd->fdbuf->code == CODE_DATA_LAST) {
		fprintf(stdout, "%s",ftpd->fdbuf->data);
		client_status_change(ftpd, STAT_WAIT_COMMAND);
		return 0;
	} else {
		fprintf(stderr,"Error Illegal Message code in stat_data_wait.\n");
		exit(1);
	}
	return 0;

}



static int send_ftph(struct ftpd * ftpd, uint8_t type, uint8_t code, uint16_t datalength) {
	struct ftphead ftph;
	ftph.type = type;
	ftph.code = code;
	ftph.length = datalength;
	if (send(ftpd->s, &ftph, sizeof(struct ftphead), 0) < 0) {
		fprintf(stderr, "send error\n");
		exit(1);
	}
	print_packet(&ftph);
	return 0;
}
static int send_ftpmsg(struct ftpd * ftpd, uint8_t type, uint8_t code, char *data, uint16_t datalength) {
	struct ftpdata ftppacket;
	ftppacket.type = type;
	ftppacket.code = code;
	ftppacket.length = datalength;
	strncpy(ftppacket.data, data, datalength);
	if (send(ftpd->s, &ftppacket, sizeof(struct ftpdata), 0) < 0) {
		fprintf(stderr, "send error\n");
		exit(1);
	}
	print_packet((struct ftphead *)&ftppacket);
	return 0;
}
/*static int send_ftpdata(struct ftpd * ftpd, char *data, uint16_t datalength) {
	struct ftpdata ftppacket;
	int datasize = strlen(data);
	if (datalength != datasize) {
		fprintf(stderr, "In send data. datalen and data size are not equal.\n");
		exit(1);
	}
	int sendnum = 0;
	while (datasize <= 0) { 
		datasize -= DATASIZE;
		sendnum++;
	}
	int sendtime;
	for (sendtime = 0;sendnum >= 1;sendtime++) {
		if (sendnum == 1) {	//last data packet
			ftppacket.type = FTPMSG_DATA;
			ftppacket.code = CODE_DATA_LAST;
			ftppacket.length = datasize - (DATASIZE * sendtime);
			strncpy(ftppacket.data, data + (DATASIZE * sendtime), datalength);
		}
		else {
			ftppacket.type = FTPMSG_DATA;
			ftppacket.code = CODE_DATA_NOTLAST;
			ftppacket.length = DATASIZE;
			strncpy(ftppacket.data, data + (DATASIZE * sendtime), datalength);
		}
		if (send(ftpd->s, &ftppacket, sizeof(struct ftpdata), 0) < 0) {
			fprintf(stderr, "send error\n");
			exit(1);
		}
		sendnum--;
	}
	print_packet((struct ftphead *)&ftppacket);
	return 0;
}*/
static int send_ftpdata(struct ftpd * ftpd, char *data, uint16_t datalength) {
	struct ftpdata ftppacket;
	int datasize = strlen(data);
	if (datalength != datasize) {
		fprintf(stderr, "In send data. datalen and data size are not equal. datalength:%d datasize%d\n", datalength, datasize);
		//exit(1);
		datasize = datalength;
	}
	int sendnum = 0;
	int testdatasize = datasize;
	while (testdatasize > 0) {
		testdatasize -= DATASIZE;
		sendnum++;
	}
	int sendtime;
	for (sendtime = 0;sendnum >= 1;sendtime++) {
		memset(ftppacket.data, '\0',DATASIZE);
		if (sendnum == 1) {	//last data packet
			ftppacket.type = FTPMSG_DATA;
			ftppacket.code = CODE_DATA_LAST;
			ftppacket.length = datasize - (DATASIZE * sendtime);
			strncpy(ftppacket.data, data + (DATASIZE * sendtime), datalength - (DATASIZE * sendtime));
		}
		else {
			ftppacket.type = FTPMSG_DATA;
			ftppacket.code = CODE_DATA_NOTLAST;
			ftppacket.length = DATASIZE;
			strncpy(ftppacket.data, data + (DATASIZE * sendtime), DATASIZE);
	//		strncpy(ftppacket.data, data + (DATASIZE * sendtime), datalength);
		}
		if (send(ftpd->s, &ftppacket, sizeof(struct ftpdata), 0) < 0) {
			fprintf(stderr, "senderror\n");
			exit(1);
		}
		sendnum--;
		print_packet((struct ftphead *)&ftppacket);
	}
	return 0;
}
static int getfilesize(FILE * fp) {
	int filesize = 0;
	if (fp == NULL) return -1;
	fseek(fp, 0, SEEK_END);
	filesize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	return filesize;
}
static int client_status_change(struct ftpd * ftpd, int status) {
	ftpd->status = status;
	return 0;
}
static void myinput(int* ac, char ** av, char * input) {
  fgets(input, 512, stdin);
  input[strlen(input) - 1] = '\0';
  //memset(input, '\0', strlen(input));
  char c;
  *ac = 0;
  int flagchar = 1;
  int avnum = 0;
  for (c = input[0]; c != '\0'; *input++) {
  	c = input[0];
    if (avnum > 16) break;
    if(c != ' ' && c != '\t' && flagchar == 1) {
      av[avnum] = input;
      avnum++;
      (*ac)++;
      flagchar = 0;
    } else if (flagchar == 0 && (c == ' ' || c == '\t')) {
	     input[0] = '\0';
       flagchar = 1;
    }
  }
  return;
}

static void quit_proc(struct ftpd * ftpd, int argc, char *argv[]) {
	send_ftph(ftpd, FTPMSG_QUIT, 0, 0);
	close(ftpd->s);
	exit(1);
	return;
}
static void pwd_proc(struct ftpd * ftpd, int argc, char *argv[]) {
	client_status_change(ftpd, STAT_WAIT_PWD);
	send_ftph(ftpd, FTPMSG_PWD, 0, 0);
	return;
}
static void cd_proc(struct ftpd * ftpd, int argc, char *argv[]) {
	if (argc != 2) {
		fprintf(stderr, "Usage: cd <server-path>\n");
		return;
	}
	int pathlen = strlen(argv[1]);
	send_ftpmsg(ftpd, FTPMSG_CWD, 0, argv[1], pathlen);
	client_status_change(ftpd, STAT_WAIT_CWD);
	return;
}
static void dir_proc(struct ftpd * ftpd, int argc, char *argv[]) {
	if (argc >= 3) {
		fprintf(stderr, "Usage: dir or dir <server-dir>\n");
		return;
	}
	if (argc == 1) {
		send_ftph(ftpd, FTPMSG_LIST, 0, 0);
	} else {
		int pathlen = strlen(argv[1]);
		send_ftpmsg(ftpd, FTPMSG_LIST, 0, argv[1], pathlen);
	}
	client_status_change(ftpd, STAT_WAIT_LIST_OK);
	return;
}
static void lpwd_proc(struct ftpd * ftpd, int argc, char *argv[]) {
	if (argc != 1) {
		fprintf(stderr, "Usage: lpwd\n");
		return;
	}
	char pathname[PATHLENGTH];
	memset(pathname, '\0', PATHLENGTH);
	getcwd(pathname, PATHLENGTH);
	fprintf(stdout, "Client current directory: %s\n",pathname);
//	send_ftph(ftpd, FTPMSG_PWD, 0, 0);
	return;
}
static void lcd_proc(struct ftpd * ftpd, int argc, char *argv[]) {
	if (argc != 2) {
		fprintf(stderr, "Usage: lcd <directory>\n");
		return;
	}
	int pathlen = strlen(argv[1]);
	if (chdir(argv[1]) < 0) {
		fprintf(stderr, "Change directory error, Illegal path.\n");
	}
	return;
}
static void ldir_proc(struct ftpd * ftpd, int argc, char *argv[]) {
	if (argc >= 3) {
		fprintf(stderr, "Usage: ldir or ldir <server-dir>\n");
		return;
	}
	int pid;
	if (argc == 1) {
		char * command = "ls";
		char * arg[3];
		arg[0] = " ";
		arg[1] = "-l";
		arg[2] = NULL;
		if ((pid = fork()) == 0) execvp(command, arg);
	} else {
		char * command = "ls";
		char * arg[4];
		arg[0] = " ";
		arg[1] = "-l";
		arg[2] = argv[1];
		arg[3] = NULL;
		if ((pid = fork()) == 0) execvp(command, arg);
	}
	wait(&pid);
	return;
}
static void get_proc(struct ftpd * ftpd, int argc, char *argv[]) {
	if (argc >= 4 || argc == 1) {
		fprintf(stderr, "Usage:  get <server file> or get <server file> <client file>\n");
		return;
	}
	if (argc == 2) {
		if((ftpd->fp = fopen(argv[1], "wb")) == NULL) {
			fprintf(stderr, "File access prohibited\n");
			return;
		}
	} else if (argc == 3) {
		if((ftpd->fp = fopen(argv[2], "wb")) == NULL) {
			fprintf(stderr, "File access prohibited\n");
			return;
		}
	}
	int pathlen = strlen(argv[1]);
	send_ftpmsg(ftpd, FTPMSG_RETR, 0, argv[1], pathlen);
	client_status_change(ftpd, STAT_WAIT_RETR_OK);
	return;
}
static void put_proc(struct ftpd * ftpd, int argc, char *argv[]) {
	if (argc >= 4 || argc == 1) {
		fprintf(stderr, "Usage: put <client file> or put <client file> <server file>\n");
		return;
	}
	if (argc == 2) {
		if((ftpd->fp = fopen(argv[1], "rb")) == NULL) {
			fprintf(stderr, "File access prohibited\n");
			return;
		}
		int pathlen = strlen(argv[1]);
		send_ftpmsg(ftpd, FTPMSG_STOR, 0, argv[1], pathlen);
	} else if (argc == 3) {
		if((ftpd->fp = fopen(argv[1], "rb")) == NULL) {
			fprintf(stderr, "File access prohibited\n");
			return;
		}
		int pathlen = strlen(argv[2]);
		send_ftpmsg(ftpd, FTPMSG_STOR, 0, argv[2], pathlen);
	}
	client_status_change(ftpd, STAT_WAIT_STOR_OK);
	return;

}
static void help_proc(struct ftpd * ftpd, int argc, char *argv[]){
  fprintf(stdout, "\n");
  fprintf(stdout,"myFTP 20170126\nUsage:<command> [<args>]\n");
  fprintf(stdout,"myFTP client commands are:\n");
  fprintf(stdout, "help             Show help for this program.\n");
  fprintf(stdout, "pwd              Show server current working directory.\n");
  fprintf(stdout, "cd [path]        Change server working directory to path.\n");
  fprintf(stdout, "dir ([path])     Show file details in current server directory or path directory\n");
  fprintf(stdout, "lcd [path]       Change client working directory to path.\n");
  fprintf(stdout, "ldir ([path])    Show file details in current client directory or path directory\n");
  fprintf(stdout, "get [path1] ([path2]) Get file path1 from server as path2.\n");
  fprintf(stdout, "put [path1] ([path2]) Put file path1 to server as path2.\n");
  fprintf(stdout, "quit             Quit this program.\n");
	return;
}

static void print_packet(struct ftphead * packet) {
	char type[16];
	char code[16];
	
	char typequit[16] = "QUIT\n";
	char typepwd[16] = "PWD\n";
	char typecwd[16] = "CWD\n";
	char typelist[16] = "LIST\n";
	char typeretr[16] = "RETR\n";
	char typestor[16] = "STOR\n";
	char typeok[16] = "OK\n";
	char typecmd_err[16] = "CMD-ERR\n";
	char typesfile_err[16] = "FILE-ERR\n";
	char typeunkwn_err[16] = "UNKWN-ERR\n";
	char typedata[16] = "DATA\n";
	switch (packet->type) {
		case FTPMSG_QUIT:
			strncpy(type, typequit,14);
		break;
		case FTPMSG_PWD:
			strncpy(type, typepwd,14);
		break;
		case FTPMSG_CWD:
			strncpy(type, typecwd,14);
		break;
		case FTPMSG_LIST:
			strncpy(type, typelist,14);
		break;
		case FTPMSG_RETR:
			strncpy(type, typeretr,14);
		break;
		case FTPMSG_STOR:
			strncpy(type, typestor,14);
		break;
		case FTPMSG_OK:
			strncpy(type, typeok,14);
		break;
		case FTPMSG_CMD_ERR:
			strncpy(type, typecmd_err,14);
		break;
		case FTPMSG_FILE_ERR:
			strncpy(type, typesfile_err,14);
		break;
		case FTPMSG_UNKWN_ERR:
			strncpy(type, typeunkwn_err,14);
		break;
		case FTPMSG_DATA:
			strncpy(type, typedata,14);
		break;
		default:
		break;
	}
	fprintf(stderr, "\n\n");
	fprintf(stderr, "=======Send packet=====\n");
	fprintf(stderr, "Type: 0x%02x %s", packet->type, type);
	fprintf(stderr, "Code: 0x%02x\n", packet->code);
	fprintf(stderr, "Length: %d\n", packet->length);
	fprintf(stderr, "=======================\n");
	return;
}
static void print_received_packet(struct ftpd * ftpd) {
	char type[16];
	char code[16];
	
	char typequit[16] = "QUIT\n";
	char typepwd[16] = "PWD\n";
	char typecwd[16] = "CWD\n";
	char typelist[16] = "LIST\n";
	char typeretr[16] = "RETR\n";
	char typestor[16] = "STOR\n";
	char typeok[16] = "OK\n";
	char typecmd_err[16] = "CMD-ERR\n";
	char typesfile_err[16] = "FILE-ERR\n";
	char typeunkwn_err[16] = "UNKWN-ERR\n";
	char typedata[16] = "DATA\n";
	switch (ftpd->fdbuf->type) {
		case FTPMSG_QUIT:
			strncpy(type, typequit,14);
		break;
		case FTPMSG_PWD:
			strncpy(type, typepwd,14);
		break;
		case FTPMSG_CWD:
			strncpy(type, typecwd,14);
		break;
		case FTPMSG_LIST:
			strncpy(type, typelist,14);
		break;
		case FTPMSG_RETR:
			strncpy(type, typeretr,14);
		break;
		case FTPMSG_STOR:
			strncpy(type, typestor,14);
		break;
		case FTPMSG_OK:
			strncpy(type, typeok,14);
		break;
		case FTPMSG_CMD_ERR:
			strncpy(type, typecmd_err,14);
		break;
		case FTPMSG_FILE_ERR:
			strncpy(type, typesfile_err,14);
		break;
		case FTPMSG_UNKWN_ERR:
			strncpy(type, typeunkwn_err,14);
		break;
		case FTPMSG_DATA:
			strncpy(type, typedata,14);
		break;
		default:
		break;
	}
	fprintf(stderr, "\n\n");
	fprintf(stderr, "===Received packet====\n");
	fprintf(stderr, "Type: 0x%02x %s", ftpd->fdbuf->type, type);
	fprintf(stderr, "Code: 0x%02x\n", ftpd->fdbuf->code);
	fprintf(stderr, "Length: %d\n", ftpd->fdbuf->length);
	fprintf(stderr, "======================\n");
	return;
}

int main(int argc, char * argv[]) {
	struct ftpd ftpd;
	init_ftpd(&ftpd, argc, argv);
	loop_ftpd(&ftpd);
	return 0;
}



















