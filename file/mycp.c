#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
void errin(char * fname,int fd);
void errout(char * fname,int fd);
int main(int argc, char *argv[])
{
  int infd;
  int outfd;
  printf("ok");
  errin(argv[1], infd);
  errout(argv[2], outfd);
  char  inbuf[1001];
  char  outbuf[1001];
  printf("nk");
  while(read(infd, inbuf, 1000) != -1){
    write(outfd, outbuf, 1000);
  }
  close(infd);
  close(outfd);
  return 0;
}
void errin(char * fname, int fd){
  if((fd = open(fname, O_RDONLY|O_EXCL,644)) < 0) {
    if(errno != EEXIST) {
      perror("open");
      exit(1);
    }
  }
}
void errout(char * fname,int fd){
  if((fd = open(fname, O_RDWR|O_CREAT)) < 0) {
    if(errno != EEXIST) {
      perror("open");
      exit(1);
    }
    if((fd = open(fname, O_RDWR|O_CREAT|O_TRUNC))<0){
      perror("open");
      exit(1);
    }
  }
}
