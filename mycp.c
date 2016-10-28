#include <stdio.h>
#include <unistd.h>
#include <errono.h>
#include <fcntl.h>
void errin(char * fname,int fd);
void errout(char * fname,int fd);
int main(int argc, char *argv[])
{
  int infd;
  int outfd;
  errin(argv[1], infd);
  errout(argv[2], outfd);
  char  inbuf[1001];
  //  char  outbuf[1001];
  while(read(infd, inbuf, 1000) != -1){
    write(outfd, inbuf, 1000);
  }
  close(infd);
  close(outfd);
  return 0;
}
void errin(char * fname, int fd){
  if((fd = open(fname, O_RDWR|O_CREAT|O_EXCL,644)) < 0) {
    if(errono != EEXIST) {
      perror("open");
      exit(1);
    }
    if((fd = open(fname, O_RDWR|O_CREAT|O_TRUNC.0644))<0){
      perror("open");
      exit(1);
    }
  }
}
void errout(char * fname,int fd){
  if((fd = open(fname, O_RDWR|O_CREAT|O_EXCL,644)) < 0) {
    if(errono != EEXIST) {
      perror("open");
      exit(1);
    }
    if((fd = open(fname, O_RDWR|O_CREAT|O_TRUNC.0644))<0){
      perror("open");
      exit(1);
    }
  }
}
