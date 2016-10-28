#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#define SIZE 2


int main (int argc ,char *argv[]){

  int fd1, fd2;
  char buf[SIZE];

  fd1 = open(argv[1], O_RDONLY);
  if (fd1 < 0)
    return;
  fd2 = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC);
  if (fd2 < 0) {
    close(fd1);
    return;
    
  }

  while (1){
    if(read(fd1,buf,SIZE) == 0)
      break;
    write(fd2, buf, SIZE);
    
  }

  close(fd1);
  close(fd2);
}
