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

	struct addrinfo res;
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

static int send_ftph(struct ftpd * ftpd, uint8_t type, uint8_t code, uint16_t datalength);
static int send_ftpmsg(struct ftpd * ftpd, uint8_t type, uint8_t code, char *data, uint16_t datalength);
static int send_ftpdata(struct ftpd * ftpd, char *data, uint16_t datalength);

static int getfilesize(FILE * fp);
static int client_status_change(struct ftpd * ftpd, int status);
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