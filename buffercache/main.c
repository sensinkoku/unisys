//#include <list.c>
#include "testarg.h"
#include "list.h"
//#include <string.h>
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
int blknos[12] = {3,35,99,98,50,10,17,5,97,28,4,64};
//int frees[6] = {3,5,4,28,97,10}
int frees[6] = {0,7,10,9,8,5};


int main(int argc, char *argv[])
{
  int ac;
  char *av[16];
  char *input;
  struct command_table *p;
  myinput(&ac, av, input);
  for (p = cmd_tbl; p->cmd; p++)
    if (strcmp(av[0], p->cmd) == 0) {
      (*p->func)(ac, av);
      break;
    }
    if(p->cmd == NULL)
      fprintf(stderr, "unknown command: %s\n", av[0]);
  return 0;
}
void help_proc(int ac, char **av) {printf("help\n");return;}
void init_proc(int ac, char **av) {
  //intialize ponters
  int i;
  for (i = 0; i < 12; i++) {
    buffers[i].hash_fp = &buffers[i];
    buffers[i].hash_bp = &buffers[i];
    buffers[i].free_fp = &buffers[i];
    buffers[i].free_bp = &buffers[i];
    buffers[i].blkno = blknos[i];
    insert_head(&hash_head[buffers[i].blkno % NHASH], &buffers[i]);
    buffers[i].stat = 0;
    buffers[i].stat |= STAT_VALID;
    buffers[i].stat |= STAT_LOCKED;
  }
  for (i = 0; i < 6; i++) {
    insert_free(&freehead, &buffers[frees[i]], 0);
    buffers[frees[i]].stat &= ~STAT_LOCKED;
  }
  return;
}
void buf_proc(int ac, char **av){
  int i, j;
  char bitchar[6] = {'O','W','K','D','V','L'};
  if (ac == 1) {
    for (i = 0; i < 12; i++) {
      char stat[6] = {'-','-','-','-','-','-'};
      for (j = 0; j < 6; j++) {
          if (buffers[i].stat & 0x01 << j) stat[j] = bitchar[j];
      }
    printf("[ %d: %d ", i, buffers[i].blkno);
    for (j = 0; j < 6; j++) printf("%c", stat[j]);
    printf("]\n");
    }
  }

  return;}
void hash_proc(int ac, char **av){return;}
void free_proc(int ac, char **av){printf("free\n");return;}
void getblk_proc(int ac, char **av){return;}
void brelse_proc(int ac, char **av){return;}
void reset_proc(int ac, char **av){return;}
void quit_proc(int ac, char **av) {return;}
void set_proc(int ac, char **av){return;}

