#include <stdio.h>
#include <list.c>
void help_proc(int, char *[]), init_proc(int, char *[]), buf_proc(int, char *[]);
void hash_proc(int, char *[]), free_proc(), getblk_proc(int), brelse_proc(int);
void reset_proc(int, char *[]), quit_proc(), set_proc(int, char * []);

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
int main(int argc, char *argv[])
{
  int ac;
  char *av[16];
  char *input;
  int i;
  scanf("%s", input);
  char c;
  for (c = input[0];c != '\0';input++) {
    int flagchar = 1;
    int avnum = 0;
    if(c != ' ' && c != '\t' && flagchar == 1) {
      av[avnum] = input;
      avnum++;
      flagchar = 0;
    } else if (flagchar == 0 && (c == ' ' || c == '\t'){
	input[0] = '\0';
    }
    
  }

  
  return 0;
}
