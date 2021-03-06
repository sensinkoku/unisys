#include <stdlib.h>
#include "testarg.h"
#include "list.h"
void help_proc(int, char *[]);
void init_proc(int, char *[]);
void buf_proc(int, char *[]);
void hash_proc(int, char *[]);
void free_proc(int , char *[]);
void getblk_proc(int, char *[]);
void brelse_proc(int, char *[]);
void reset_proc(int, char *[]);
void quit_proc(int , char * []);
void set_proc(int, char * []);
void init();
struct command_table{
  char * cmd;
  void (*func)(int, char *[]);
} cmd_tbl[] = {
  {"help", help_proc},
  {"init", init_proc},
  {"buf", buf_proc},
  {"hash", hash_proc},
  {"free", free_proc},
  {"getblk", getblk_proc},
  {"brelse", brelse_proc},
  {"set", set_proc},
  {"reset", reset_proc},
  {"quit", quit_proc},
  {NULL, NULL}
};
struct buf_header hash_head[NHASH];
struct buf_header freehead;
struct buf_header buffers[12];
int blknos[12] = {28,4,64,17,5,97,98,50,10,3,35,99};
int frees[6] = {9,4,1,0,5,8};
int insertorder[12] = {2,1,0,5,4,3,8,7,6,11,10,9};

int main(int argc, char *argv[])
{
  int ac;
  char *av[16];
  char *input;
  struct command_table *p;
  init();
  while(1){
      printf("$ ");
    myinput(&ac, av, input);
    for (p = cmd_tbl; p->cmd; p++)
      if (strcmp(av[0], p->cmd) == 0) {
        (*p->func)(ac, av);
	       break;
      }
    if(p->cmd == NULL)
      fprintf(stderr, "unknown command: %s\n", av[0]);
  }
  return 0;
}
void help_proc(int ac, char **av) {
  printf("Buffercache 20161102\n\nUsage:<command> [<args>]\n");
  printf("Buffercache command are:\n");
  printf("help             Show help for this command\n");
  printf("buf [n..]        Show the states of buffer you designate in buffer numbers. If no arguments, show all buffer states.\n");
  printf("hash [n..]       Show hashlists you designate in hash numbers. If no arguments, show all buffer states\n");
  printf("free             Show freelist\n");
  printf("getblk n         Execute getblk(n), requiring buffernumber, n.\n");
  printf("brelse n         Execute brelse(n), requiring buffernumber, n.\n");
  printf("set n [stat..]   Set states to buffernumber n's buffer\n");
  printf("reset n [stat..] Reset states to buffernumber n's buffer\n");
  printf("init             Initialize buffers to default sample state.\n");
  printf("quit             Quit this command.\n");
  return;
}
void init(){
  //intialize ponters
  int i;
  for(i = 0; i < NHASH; i++) {
    hash_head[i].hash_fp = &hash_head[i];
    hash_head[i].hash_bp = &hash_head[i];
    hash_head[i].free_fp = &hash_head[i];
    hash_head[i].free_fp = &hash_head[i];
  }
  freehead.hash_fp = &freehead;
  freehead.hash_bp = &freehead;
  freehead.free_fp = &freehead;
  freehead.free_bp = &freehead;
  for (i = 0; i < 12; i++) {
    buffers[i].hash_fp = &buffers[i];
    buffers[i].hash_bp = &buffers[i];
    buffers[i].free_fp = &buffers[i];
    buffers[i].free_bp = &buffers[i];
    buffers[i].blkno = blknos[i];
    buffers[i].bufnum = i;
    buffers[i].stat = 0;
    buffers[i].stat |= STAT_VALID;
    buffers[i].stat |= STAT_LOCKED;
  }
  for (i = 0; i < 12; i++) {
    insert_head(&hash_head[buffers[insertorder[i]].blkno % NHASH], &buffers[insertorder[i]]);
      }
  for (i = 0; i < 6; i++) {
    insert_free(&freehead, &buffers[frees[i]], 0);
    buffers[frees[i]].stat &= ~STAT_LOCKED;
  }
  return;  
}
void init_proc(int ac, char **av) {
  init();
  return;
}
void buf_proc(int ac, char **av){
int i;
  if (ac == 1) {
    for (i = 0; i < 12; i++) {
      buf_state_print(&buffers[i]);
      printf("\n");
    }
    return;
  }
  for (i = 1; i < ac; i++) {
    int a = atoi(av[i]);
    if (a < 0 || 12 <= a) {
      fprintf(stderr,"Illegal arguments\n");
      return;
      }
      buf_state_print(&buffers[a]);
      printf("\n");
    }
  return;
}
void hash_proc(int ac, char **av){
  int i;
  if (ac == 1) {
    for (i = 0; i < NHASH; i++) printhash(i);
    return;
  }
  for (i = 1;i < ac;i++ ){
    printhash(atoi(av[i]));
  }  
  return;
}
void free_proc(int ac, char **av){
   printfree();
  return;
}
void getblk_proc(int ac, char **av){
  if (ac != 2) fprintf(stderr,"Usage: getblk <block number>\n");
  else
    getblk(atoi(av[1]));
  return;
}
void brelse_proc(int ac, char **av){
  struct buf_header * p;
   if (ac != 2) fprintf(stderr,"Usage: brelse <block number>\n");
  else{
    if ((p = getblkpointer(atoi(av[1]))) == NULL) 
      fprintf(stderr,"This buffer is not cached\n");
    else
      brelse(p);
  }
  return;}
void reset_proc(int ac, char **av){
   int i;
  if (ac < 3) {
    fprintf(stderr,"Usage:set <block number> <state>");
    return;
  }
  int blkno = atoi(av[1]);
  if (search_hash(blkno) == NULL) {
    fprintf(stderr,"This block is not on buffer.\n");
    return;
  }
  for (i = 2; i < ac; i++) {
    if (strlen(av[i]) != 1) {
      fprintf(stderr,"This parameter is not valid.\n");
      return;
    }
    if (av[i][0] != 'L'&&av[i][0]!='V'&&av[i][0]!='D'&&av[i][0]!='K'&&av[i][0]!='W'&&av[i][0]!='O'){
      fprintf(stderr,"This parameter is not valid.\n");
      return;
    }
  }
  bufresetstat(ac, av);
  return;
}
void quit_proc(int ac, char **av) {exit(0);return;}
void set_proc(int ac, char **av) {
  int i;
  if (ac < 3) {
    fprintf (stderr,"Usage:set <block number> <state>");
    return;
  }
  int blkno = atoi(av[1]);
  if (search_hash(blkno) == NULL) {
    fprintf(stderr,"This block is not on buffer.\n");
    return;
  }
  for (i = 2; i < ac; i++) {
    if (strlen(av[i]) != 1) {
      fprintf(stderr,"This parameter is not valid.\n");
      return;
    }
    if (av[i][0] != 'L'&&av[i][0]!='V'&&av[i][0]!='D'&&av[i][0]!='K'&&av[i][0]!='W'&&av[i][0]!='O'){
      fprintf(stderr,"This parameter is not valid.\n");
      return;
    }
  }
  bufsetstat(ac, av);
  return;
}

