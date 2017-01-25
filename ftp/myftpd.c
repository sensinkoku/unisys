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
	FILE * fp;
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
static int server_status_change(struct ftpd * ftpd, int status);
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
		make_connection(ftpd);
		int chpid; // chile process id;
		//if ((chpid = fork()) == 0) {
			fprintf(stderr, "TCP connection success. Forked process.\n");
			ftp_work(ftpd);
		//}
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
		if ((ftpd->clis = accept(ftpd->s, (struct sockaddr *)&(ftpd->cliskt), &sktlen)) < 0) {
			perror("accept error");
			exit(1);
		}
		ftpd->status = STAT_TCP_CONNECT;
		return 0;
}

static int ftp_work(struct ftpd * ftpd) {
	for(;;){
		int count;
		if ((count = (recv(ftpd->clis, ftpd->fdbuf, sizeof (struct ftpdata), 0))) < 0) {
			perror("recvfrom error");
			exit(1);
		} else if (count == 0) {
			fprintf(stderr, "TCP Connection finished. Program exit.\n");
			exit(1);
		}

		print_received_packet(ftpd);
		if (ftpd->status == STAT_TCP_CONNECT) {
			work_in_stat_connect(ftpd);
		} else if (ftpd->status == STAT_RECVMSG) {
			work_in_recvmsg(ftpd);
		}
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
	/*char *cdcom[3];
	cdcom[0] = "cd";
	cdcom[1] = ftpd->fdbuf->length;
	cdcom[2] = NULL;*/
	char * cd = (char *) malloc(5+pathlen);
	strncpy(cd, "cd ", 3);
	//memset(path, '\0', pathlen+1);
	char pathname[PATHLENGTH];
	strncpy(path, ftpd->fdbuf->data, pathlen);
	strncat(cd, path, pathlen);
	getcwd(pathname, PATHLENGTH);
	strncat(pathname, "/", pathlen);
	strncat(pathname, path, pathlen);
	fprintf(stderr, "cdpath:%s\n", pathname);
	if (chdir(pathname) < 0) {
		fprintf(stderr, "Change directory error, Illegal path.\n");
		send_ftph(ftpd, FTPMSG_FILE_ERR, CODE_FILEERR_NODIR,0);

	} else { //set path
		free(ftpd->path);
		
		getcwd(pathname, PATHLENGTH);
		int pathlength = strlen(pathname);

		ftpd->pathlength = pathlength;
		ftpd->path = (char *)malloc ((pathlength+1) * sizeof(char));
		memset(ftpd->path, '\0', ftpd->pathlength+1);
		strncpy(ftpd->path,pathname,pathlength);
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
	char buff[2048];
	int filesize = 0;
	while (fgets(buff, sizeof(buff), fp)) {
		fprintf(stderr, "fgets:%s", buff);
		filesize += strlen(buff);
	}

	char data[filesize];
	memset (data, 0, filesize);
	pclose(fp);
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
	while (fgets(buff, sizeof(buff), fp)) {
		fprintf(stderr, "fgets:%s", buff);
		int size = strlen(buff);
		strncat(data, buff, size);
	}
	fprintf(stderr, "%d\n", filesize);
	fprintf(stderr, "%d\n", strlen(data));
	//int filesize = getfilesize(fp);
	//fprintf(stderr, "filesize :%d\n", filesize);

/*	char data[filesize];
fprintf(stderr, "filesize :%d\n", filesize);
fprintf(stderr, "data:%s", data);
	fread(data, sizeof(char), filesize, fp);
fprintf(stderr, "filesize :%d\n", filesize);
fprintf(stderr, "data:%s", data);
	send_ftpdata(ftpd, data, filesize);*/
	send_ftpdata(ftpd, data, filesize);
	pclose(fp);
	return 0;
}
static int msg_retr(struct ftpd * ftpd) {
	int pathlen = ftpd->fdbuf->length;
	char path [pathlen];
	//memset(path, '\0', pathlen+1);
	strncpy(path, ftpd->fdbuf->data, pathlen);
	FILE *fp;
	if ((fp = fopen(path, "r")) == NULL) {
		fprintf(stderr, "no such file in server\n");
		send_ftph(ftpd,FTPMSG_FILE_ERR,CODE_FILEERR_NOACCESS,0);
		return -1;
	}
	send_ftph(ftpd, FTPMSG_OK, CODE_OK_DATA_STOC, 0);
	int filesize = getfilesize(fp);
	char *data = (char *) malloc(filesize * sizeof(char));
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
	if ((ftpd->fp = fopen(path, "bw")) == NULL) {
		fprintf(stderr, "File access prohibited\n");
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
static int msg_fileerr(struct ftpd * ftpd);
static int msg_unkwnerr(struct ftpd * ftpd);
static int msg_data(struct ftpd * ftpd) {
	fwrite(ftpd->fdbuf->data, sizeof (char), ftpd->fdbuf->length, ftpd->fp);
	if (ftpd->fdbuf->code == CODE_DATA_LAST) {
		server_status_change(ftpd, STAT_TCP_CONNECT);
		fclose(ftpd->fp);
	}
	return 0;
}

static int send_ftph(struct ftpd * ftpd, uint8_t type, uint8_t code, uint16_t datalength) {
	struct ftphead ftph;
	ftph.type = type;
	ftph.code = code;
	ftph.length = datalength;
	if (send(ftpd->clis, &ftph, sizeof(struct ftphead), 0) < 0) {
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
	if (send(ftpd->clis, &ftppacket, sizeof(struct ftpdata), 0) < 0) {
		fprintf(stderr, "send error\n");
		exit(1);
	}
	print_packet((struct ftphead *)&ftppacket);
	return 0;
}
static int send_ftpdata(struct ftpd * ftpd, char *data, uint16_t datalength) {
	struct ftpdata ftppacket;
	int datasize = strlen(data);
	if (datalength != datasize) {
		fprintf(stderr, "In send data. datalen and data size are not equal.\n");
		//exit(1);
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
		if (send(ftpd->clis, &ftppacket, sizeof(struct ftpdata), 0) < 0) {
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
//	fgetpos(fp, &filesize);
	fseek(fp, 0, SEEK_SET);
	fprintf(stderr, "filesize is %d\n", filesize);
	return filesize;
}
static int server_status_change(struct ftpd * ftpd, int status) {
	ftpd->status = status;
	return 0;
}
static void print_packet(struct ftphead * packet) {
	fprintf(stderr, "\n\nSend packet.\n");
	fprintf(stderr, "======================\n");
	fprintf(stderr, "Type: %d\n", packet->type);
	fprintf(stderr, "Code: %d\n", packet->code);
	fprintf(stderr, "Length: %d\n", packet->length);
	fprintf(stderr, "======================\n");
	return;
}
static void print_received_packet(struct ftpd * ftpd) {
	fprintf(stderr, "\n\nReceived packet.\n");
	fprintf(stderr, "======================\n");
	fprintf(stderr, "Type: %d\n", ftpd->fdbuf->type);
	fprintf(stderr, "Code: %d\n", ftpd->fdbuf->code);
	fprintf(stderr, "Length: %d\n", ftpd->fdbuf->length);
	fprintf(stderr, "======================\n");
	return;
}
int main(int argc, char * argv[]) {
	struct ftpd ftpd;
	init_ftpd(&ftpd, argc, argv);
	fprintf(stderr, "Path is %s\n", ftpd.path);
	loop_ftpd(&ftpd);
	return 0;
}



















