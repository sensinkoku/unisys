#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#define LEN 100
void myinput(int* ac, char ** av, char * input, int *ispipe, int *isredirect);
static int redirect_check(char **av, int startindex, int endindex, int * inputplace, int * outputplace);
int main(int argc, char *argv[], char *envs[])
{
signal(SIGINT,SIG_IGN);
  int ac;
  char *av[256];
  char input[1024];
  int ispipe[ac];
  int isredirect[ac];
  int i;
  int pipenum = 0;
  	int fdtty = open("/dev/tty", O_RDWR);
  while(1) {
  	  int status[pipenum+1];
    printf("$ ");
    myinput(&ac, av, input, ispipe, isredirect);
    if (ac >=1){
	int i;
	int pipeplace;
	int pipefile[pipenum][2];
	int waitflag = 1;
	int chpid;
	int chgid;
	int shellppid = getppid();
	for (i = 0; i < ac; i++) printf("this%d this",ispipe[i]);
	if (ispipe[0] != 0 || ispipe[ac-1] != 0) fprintf(stderr, "Input error\n");
	pipenum = 0;
	for (i = 0; i < ac; i++) if (ispipe[i] == 1) pipenum++;
	if (strcmp(av[0],"cd") == 0 && pipenum == 0) {
        char *  home = getenv("HOME");
        if (ac == 1 || strcmp(av[1], "~") == 0) chdir(home);
        else chdir(av[1]);
      } else if (strcmp(av[ac-1],"exit") == 0 && pipenum == 0) {
	exit(0);
      } else {
	printf("\n%d %d\n",ac, pipenum);
	int j;
	j = 0;
	printf("pipenum %d\n",pipenum);
	int k1;
	for (i = 0; i < pipenum+1; i++) {
	  int start, end;
	  int comstart, comend;
	  int inputflag, outputflag;
	  int redirect_in_index, redirect_out_index;
	  int fd1, fd2;
	  if (i == 0) start = 0;
	  else {
	  	start = j+1;
	  	j++;
	  }
	  while(ispipe[j] != 1 && j != ac-1) j++;
	  printf("j is %d\n",j);
	  if (ispipe[j] == 1) outputflag = 1;
	  else outputflag = 0;
	  if (i != 0) inputflag = 1;
	  if (i == 0) inputflag = 0;
	  else inputflag = 1;
	  //pipeopen
	  int r;
	  if(ispipe[j] == 1) {
	  	r = pipe(pipefile[i]);
	  	if (r == -1) {fprintf(stderr,"pipe error\n"); return -1;}
	  }
	  // redirect check
	  printf("opf = %d\n",outputflag);
	  if (outputflag == 1) end = j-1;
	   else end = j;
	  if (redirect_check(av, start,end,&redirect_in_index,&redirect_out_index) == -1) {
	    fprintf(stderr,"Too many redirects\n");
	    break;
	  }
	  if (redirect_in_index != -1) inputflag = 2;
	  if (redirect_out_index != -1) outputflag = 2;
	  comend = end;
	  comstart = start;
	  if ((chpid =fork()) == 0) {
	  	signal(SIGINT,SIG_DFL);
	  	if (i == 0) chgid = chpid;
	  	int m;
	    switch (inputflag) {
	    case 0:{
	    	fprintf(stderr,"input-0\n");
	    break;
	    }
	    case 1: {
	    fprintf(stderr,"input-1\n");
	      close(0);
	      dup(pipefile[i-1][0]);
	  	close(pipefile[i-1][0]);
	  	close(pipefile[i-1][1]);
	      break;
	    }
	    case 2: {
	    	fprintf(stderr,"input-2\n");
	      comend = redirect_in_index - 1;
	      fprintf(stderr,"red in index %d\n", redirect_in_index);
	      fd1 = open(av[redirect_in_index+1],O_RDONLY, 0644);
	      close(0);
	      dup(fd1);
	      close(fd1);
	      break;
	    }
	    default:
	    fprintf(stderr,"input-def\n");
	      break;
	    }

	    switch (outputflag) {
	    case 0: {
	    	fprintf(stderr,"output-0\n");
	      break;
	    }
	    case 1: {
	    		fprintf(stderr,"output-1\n");
	      close(1);
	      dup(pipefile[i][1]);
	       close(pipefile[i][0]);
	       close(pipefile[i][1]);
	      break;
	    }
	    case 2: {
	    		fprintf(stderr,"output-2\n");
	      comend = redirect_out_index - 1;
	      fd2 = open(av[redirect_out_index+1],O_WRONLY|O_CREAT|O_TRUNC, 0644);
	      close(1);
	      dup(fd2);
	      close(fd2);
	      break;
	    }
	    default:
	    	fprintf(stderr,"output-def\n");
	      break;
	    }
	    //exe command
	    int comlength = comend-comstart+1;
	    char * exein[comlength+1];
	    int k;
	    for (k = 0; k < comlength; k++) {
	      exein[k] = av[comstart+k];
	    }
	    fprintf (stderr,"comlength is %d\n", comend-comstart+1);
	    exein[comlength] = NULL;
	    if (strcmp(exein[comlength-1],"&")== 0) exein[comlength-1] = NULL;
	    fprintf(stderr,"exe %s\n", exein[0]);

	    if (redirect_in_index != -1) {
	  	if(strcmp(av[ac-3], "&") == 0) waitflag = 0;
	    }
	    if(strcmp(av[ac-1], "&")== 0) waitflag = 0;
	    if (waitflag == 0) {
	    	setpgid(0, shellppid);
	    } else {
	    	setpgid(0, chgid);
	    }
	    char ** environ;
	    char * path = getenv("PATH");
	    char * t = path;
	    char * exepath;
	    int length;
	    length = strlen(path) + strlen(av[0]) + 2;
	    exepath = (char * )malloc(sizeof(char) * length);
	    printf("%s", t);
	    if ((t = strtok(path,":"))== NULL) {
	    	strcpy(exepath, t);
	    	strcat(exepath, "/");
	    	strcat(exepath, exein[0]);
	    	fprintf(stderr, "thisis exe %s %s\n",exepath, exein[0]);
	    	execve(exepath, exein, environ);
	    } else
	    while((t = strtok(NULL,":"))!= NULL) {
	    	strcpy(exepath, t);
	    	strcat(exepath, "/");
	    	strcat(exepath, exein[0]);
	    	fprintf(stderr, "thisis exe %s %s\n",exepath, exein[0]);
	    	if (execve(exepath, exein, environ)<0) continue;
	    }
	    exit(0);
	  }//fork kakko
	  if (i != 0) {
	  	close(pipefile[i-1][0]);
	  	close(pipefile[i-1][1]);
	  }
	  int k;
	  if (redirect_in_index != -1) {
	  	if(strcmp(av[ac-3], "&") == 0) waitflag = 0;
	  }
	  if(strcmp(av[ac-1], "&")== 0) waitflag = 0;
  	if (waitflag) {
  		 tcsetpgrp(fdtty, chgid);
  		for (k = 0; k < pipenum + 1; k++) wait(&status[k]);
  	}else{
  		printf ("anpasand\n");
  		tcsetpgrp(fdtty, getpid());
  	}
  	}//for kakko
    }
    }
    tcsetpgrp(fdtty, getpid());
  }
  return 0;
}
/*static int pipefdopen(int x,int y, int array[x][y]) {
  int i;
  if (x == 0) {
    return 2;
  }
  for (i = 0; i < x; i++) {
    if(pipe(array[x]) == -1) {
      return -1;
    }
  }
  return 0;
}*/
static int redirect_check(char **av, int startindex, int endindex, int * inputplace, int * outputplace) {
  int i;
  int inp_num, oup_num;
  inp_num = 0; oup_num = 0;
  *inputplace = -1;
  *outputplace = -1;
  for (i = startindex; i < endindex+1; i++) {
    if (strcmp(av[i], "<") == 0) {
      inp_num++;
      *inputplace = i;
      if (i == endindex) {
	fprintf(stderr, "Illegal usage of redirect.\n");
	return -1;
      }
    }
    if (strcmp(av[i], ">") == 0) {
       oup_num++;
      *outputplace = i;
      if (i == startindex) {
	fprintf(stderr, "Illegal usage of redirect.\n");
	return -1;
      }
  }
  if (inp_num >= 2 || oup_num >= 2) {
    fprintf(stderr, "Illegal usage of redirect.\n");
  	}
	}
}
void myinput(int* ac, char ** av, char * input, int *ispipe, int *isredirect) {
  fgets(input, 250, stdin);
  if (strlen(input)>=1) input[strlen(input) - 1] = '\0';
  char c;
  *ac = 0;
  int flagchar = 1;
  // when flagchar is 1, waiting new arg, else during arg
  int avnum = 0;
  int i;
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
  for (i = 0; i < *ac; i++) {
    ispipe[i] = 0;
    isredirect[i] = 0;
  }
  for (i = 0; i < *ac; i++) {
    if (strcmp(av[i],"|") == 0) ispipe[i] = 1;
    if (strcmp(av[i], ">") == 0) ispipe[i] = 2;
    if (strcmp(av[i], "<") == 0) ispipe[i] = 3;
  }
  return;
}
