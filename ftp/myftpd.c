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

#define DATASIZE 1024
#define FTPPORTNUM 50021
#define PATHLENGTH 1024

#define STAT_INITIAL 0
#define STAT_TCP_CONNECT 1
#define STAT_RECVMSG 2

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
#define CODE_DATA_NOTLAST 0x00
#define CODE_DATA_LAST 0x01



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
	// below network order 
	struct sockaddr_in myskt;
	struct sockaddr_in cliskt;
	struct ftpdata *fdbuf;
	struct ftphead *fdhbuf;
};


static int init_ftpd(struct ftpd * ftpd, int argc, char *argv[]);
static int loop_ftpd(struct ftpd * ftpd);

static int loadargs(struct ftpd * ftpd, int argc, char *argv[]);
static int init_ftpd_struct(struct ftpd * ftpd);
static int setsig(struct ftpd * ftpd);
static int tcpstart(struct ftpd * ftpd);
static int make_connection(struct ftpd * ftpd);
static int ftp_work(struct ftpd * ftpd);

static int work_in_stat_connect(struct ftpd * ftpd);
static int work_in_recvmsg(struct ftpd * ftpd);

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

static int send_ftph(struct ftpd * ftpd, uint8_t type, uint8_t code, uint16_t datalength);
static int send_ftpmsg(struct ftpd * ftpd, uint8_t type, uint8_t code, char *data, uint16_t datalength);
static int send_ftpdata(struct ftpd * ftpd, char *data, uint16_t datalength);

static int getfilesize(FILE * fp);


static int init_ftpd(struct ftpd * ftpd, int argc, char *argv[]) {
	loadargs(ftpd, argc, argv);
	init_ftpd_struct(ftpd);
	setsig(ftpd);
	tcpstart(ftpd);
	return 0;
}
static int loop_ftpd(struct ftpd * ftpd) {
	for (;;) {
		make_connection(ftpd);
		int chpid; // chile process id;
		if ((chpid = fork()) == 0) {
			ftp_work(ftpd);
		}
	}
	return 0;
}

static int loadargs(struct ftpd * ftpd, int argc, char *argv[]) {
	if (argc >= 3) {
		fprintf(stderr, "Error too many arguments. Usage: ./myftpd <current dir>\n");
		exit(1);
	}
	char pathname[PATHLENGTH];
	int pathlength;
	memset(pathname, '\0', PATHLENGTH);
	if (argc == 1) {
		getcwd(pathname, PATHLENGTH);
		pathlength = strlen(pathname);
	}
	if (argc == 2) {
		pathlength = strlen(argv[1]);
		strncpy(pathname, argv[1], pathlength);
	}
	ftpd->pathlength = pathlength;
	ftpd->path = (char *)malloc ((pathlength+1) * sizeof(char));
	memset(ftpd->path, '\0', pathlength+1);
	strncpy(ftpd->path,pathname,pathlength);
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

static int tcpstart(struct ftpd * ftpd) {
	if ((ftpd->s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket error");
		exit(1);
	}
	in_port_t portnum = FTPPORTNUM;
	ftpd->myskt.sin_family = AF_INET;
	ftpd->myskt.sin_port = htons(portnum);
	ftpd->myskt.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(ftpd->s, (struct sockaddr *)&(ftpd->myskt), sizeof ftpd->myskt) < 0) {
		perror("bind error");
		exit(1);
	}
	if (listen(ftpd->s, 5) < 0) {
		perror("listen error");
		exit(1);
	}
	return 0;
}

static int make_connection(struct ftpd * ftpd) {
		socklen_t sktlen;
		sktlen = sizeof ftpd->cliskt;
		if ((ftpd->clis = accept(ftpd->s, (struct sockaddr *)&(ftpd->cliskt), &sktlen) < 0) < 0) {
			perror("accept error");
			exit(1);
		}
		ftpd->status = STAT_TCP_CONNECT;
		return 0;
}

static int ftp_work(struct ftpd * ftpd) {
		int count;
		if (count = (recv(ftpd->clis, ftpd->fdbuf, sizeof (struct ftpdata), 0)) < 0) {
			perror("recvfrom error");
			exit(1);
		}
		if (ftpd->status == STAT_TCP_CONNECT) {
			work_in_stat_connect(ftpd);
		} else if (ftpd->status == STAT_RECVMSG) {
			work_in_recvmsg(ftpd);
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
	switch (ftpd->fdhbuf->type) {
		case FTPMSG_DATA: {

		}
		break;
		default:{
			fprintf(stderr, "Not proper Message in STAT_RECVMSG\n");
			return -1;
		}
		break;
	}
	return 0;
}

static int msg_quit(struct ftpd * ftpd){
	fprintf(stderr, "QUIT MSG: QUIT TCP CONNECTION");
	close(ftpd->s);
	exit(1);
}
static int msg_pwd(struct ftpd * ftpd) {
	if (ftpd->status != STAT_TCP_CONNECT) {
		fprintf (stderr, "NOT PROPER MESSAGE PWD IN THIS STATUS.\n");
		exit(1);
	}
	send_ftpdmsg(ftpd, FTPMSG_OK, CODE_OK, ftpd->path, (uint16_t)pathlength);
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
		send_ftph(ftpd->s, FTPMSG_FILE_ERR, CODE_FILEERR_NODIR,0);

	} else { //set path
		free(ftpd->path);
		ftpd->pathlength = pathlen;
		ftpd->path = (char *)malloc ((pathlen+1) * sizeof(char));
		memset(ftpd->path, '\0', pathlength+1);
		strncpy(ftpd->path,path,pathlen);
		fprintf(stderr,"Chdir to %s", ftpd->path);
		send_ftph(ftpd->s, FTPMSG_OK, CODE_OK, 0);
	}
	return 0;
}
static int msg_list(struct ftpd * ftpd) {
	int pathlen = ftpd->fdbuf->length;
	FILE *fp;
	if (pathlen == 0) {
		if ((fp = popen("ls -l", "r")) == NULL) {
			fprintf(stderr, "ls -l error\n");
			sendftpmsg(ftpd, FTPMSG_FILE_ERR, CODE_FILEERR_NODIR, 0);
			return -1;
		}
	} else {
		char path [pathlen+1];
		memset(path, '\0', pathlen+1);
		strncpy(path, ftpd->fdbuf->data, pathlen);
		char ls[32+pathlen] = "ls -l \0";
		strncat (ls, path, pathlen+1);
		if ((fp = popen(ls , "r")) == NULL) {
			fprintf(stderr, "ls -l error\n");
			sendftpmsg(ftpd, FTPMSG_FILE_ERR, CODE_FILEERR_NODIR, 0);
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
	if ((fp = fopen(path, "br")) == NULL) {
		fprintf(stderr, "no such file in server\n");
		send_ftph(ftpd,FTPMSG_FILE_ERR,CODE_FILEERR_NOACCESS,0);
		return -1;
	}
	send_ftph(ftpd, FTPMSG_OK, CODE_OK_DATA_STOC, 0);
	int filesize = getfilesize(fp);
	char data[filesize];
	fread(data ,sizeof char,filesize,fp);
	send_ftpdata(ftpd, data, filesize);
	close(fp);
	return 0;
}
static int msg_stor(struct ftpd * ftpd) {
	int pathlen = ftpd->fdbuf->length;
	char path [pathlen];
	//memset(path, '\0', pathlen+1);
	strncpy(path, ftpd->fdbuf->data, pathlen);
	FILE *fp;
	if ((fp = fopen(path, "bw")) == NULL) {
		fprintf(stderr, "no such file in server\n");
		send_ftph(ftpd,FTPMSG_FILE_ERR,CODE_FILEERR_NOACCESS,0);
		return -1;
	}
	send_ftph(ftpd, FTPMSG_OK, CODE_OK_DATA_STOC, 0);
	int filesize = getfilesize(fp);
	char data[filesize];
	fread(data ,sizeof char,filesize,fp);
	send_ftpdata(ftpd, data, filesize);
	close(fp);
	return 0;

}
static int msg_ok(struct ftpd * ftpd) {

}
static int msg_cmderr(struct ftpd * ftpd) {

}
static int msg_fileerr(struct ftpd * ftpd);
static int msg_unkwnerr(struct ftpd * ftpd);
static int msg_data(struct ftpd * ftpd);

static int send_ftph(struct ftpd * ftpd, uint8_t type, uint8_t code, uint16_t datalength) {
	struct ftphead ftph;
	ftph.type = type;
	ftph.code = code;
	ftph.length = datalength;
	if (send(ftpd->s, &ftph, sizeof(struct ftphead), 0) < 0) {
		fprintf(stderr, "send error\n");
		exit(1);
	}
	return 0;
}
static int send_ftpmsg(struct ftpd * ftpd, uint8_t type, uint8_t code, char *data, uint16_t datalength) {
	struct ftpdata ftppacket;
	ftppacket.type = type;
	ftppacket.code = code;
	ftppacket.length = datalength;
	ftppacket.data = data;
	if (send(ftpd->s, &ftppacket, sizeof(struct ftppacket), 0) < 0) {
		fprintf(stderr, "send error\n");
		exit(1);
	}
	return 0;
}
static int send_ftpdata(struct ftpd * ftpd, char *data, uint16_t datalength) {
	struct ftpdata ftppacket;
	int datasize = sizeof (data);
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
			ftppacket.data = data + (DATASIZE * sendtime);
		}
		else {
			ftppacket.type = FTPMSG_DATA;
			ftppacket.code = CODE_DATA_NOTLAST;
			ftppacket.length = DATASIZE;
			ftppacket.data = data + (DATASIZE * sendtime);
		}
		if (send(ftpd->s, &ftppacket, sizeof(struct ftpdata), 0) < 0) {
			fprintf(stderr, "send error\n");
			exit(1);
		}
		sendnum--;
	}
	return 0;
}
static int getfilesize(FILE * fp) {
	int filesize = 0;
	if (fp == NULL) return -1;
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	return filesize;
}
int main(int argc, char * argv[]) {
	struct ftpd ftpd;
	init_ftpd(&ftpd, argc, argv);
	fprintf(stderr, "Path is %s\n", ftpd.path);
	return 0;
}



















