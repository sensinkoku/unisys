#include <stdio.h>
#define NHASH 4
#define STAT_LOCKED 0x00000001
#define STAT_VALID  0x00000002
#define STAT_DWR    0x00000004
#define STAT_KRDWR  0x00000008
#define STAT_WAITED 0x00000010
#define STAT_OLD    0x00000020
struct buf_header {
    int blkno;
    int bufnum;
    struct buf_header * hash_fp;
    struct buf_header * hash_bp;
    unsigned int stat;
    struct buf_header *free_fp;
    struct buf_header *free_bp;
    char * cache_data;
  };
void insert_head(struct buf_header *, struct buf_header *);
void insert_free(struct buf_header *, struct buf_header *, int );
void remove_from_hash(struct buf_header * );
struct buf_header * getblk(int blkno);
void brelse(struct buf_header *);
struct buf_header * getblkpointer(int blkno);
void printhash(int i);
void printfree();
void buf_state_print(struct buf_header*);
void bufsetstat(int ac, char **av);
void bufresetstat(int ac, char **av);

struct buf_header * search_hash(int blkno);
//global variables
extern struct buf_header hash_head[NHASH];
extern struct buf_header freehead;
extern struct buf_header buffers[12];

struct buf_header * search_hash(int blkno){
  int h;
  struct buf_header *p;
  h = blkno % NHASH;
  for (p = hash_head[h].hash_fp;p != &hash_head[h];p = p->hash_fp) if (p->blkno == blkno) return p;
  return NULL;
}

void insert_head(struct buf_header *h, struct buf_header *p) {
  //p is insert node, h is head node.
  p->hash_bp = h;
  p->hash_fp = h->hash_fp;
  h->hash_fp->hash_bp = p;
  h->hash_fp = p;
}
void insert_free(struct buf_header *h, struct buf_header *p, int i) {
  //p is insert node, h is head node.
  // if i == 1 : insert head of list else insert of tail
  if (i == 1) {
    p->free_bp = h;
    p->free_fp = h->free_fp;
    h->free_fp->free_bp = p;
    h->free_fp = p;
  } else {
    p->free_bp = h->free_bp;
    p->free_fp = h;
    h->free_bp->free_fp = p;
    h->free_bp = p;
  }
}
void remove_from_free(struct buf_header * p){
  p->free_bp->free_fp = p->free_fp;
  p->free_fp->free_bp = p->free_bp;
  p->free_fp = p->free_bp = p;
}
void remove_from_hash(struct buf_header * p){
  p->hash_bp->hash_fp = p->hash_fp;
  p->hash_fp->hash_bp = p->hash_bp;
  p->hash_fp = p->hash_bp = p;
}
struct buf_header * getblk(int blkno) {
  struct buf_header * p;
  struct buf_header * fp;
  fp = freehead.free_fp;
  while(1){
    if((p = search_hash(blkno)) != NULL){
      if(p->stat & STAT_LOCKED){
      	//senirio 5
	       printf("Scenario5\n");
      	//sleep();
      	printf("Process goes to sleep\n");
      	return NULL;
      }
      //senario 1
      printf("Scenario1\n");
      p->stat |= STAT_LOCKED;
      remove_from_free(p);
      return p;
    } else {
      if (freehead.free_fp == &freehead) {
        	//senario 4
          printf("Scenario4\n");
        	//sleep();
        	printf("Process goes to sleep\n");
        	return NULL;
      }
      if(fp->stat & STAT_DWR) {
      	//sinario3
	     printf("Scenario3\n");
      	//write asynchronous
       fp->stat &= ~STAT_DWR;
       fp->stat |= STAT_LOCKED;
       fp->stat |= STAT_KRDWR;
       fp->stat |= STAT_OLD;
       struct buf_header * p;
       p = fp;
       fp = fp->free_fp;
       remove_from_free(p);
      	continue;
      }
      //sinario2
      printf("Scenario2\n");
      remove_from_free(fp);
      remove_from_hash(fp);
      insert_head(&hash_head[blkno % NHASH],fp);
      fp->blkno = blkno;
      fp->stat |= STAT_LOCKED;
      fp->stat |= STAT_VALID;
      return fp;
    }
  }
  return NULL;
}
struct buf_header * getblkpointer(int blkno) {
  struct buf_header * buffer;
   for (buffer = hash_head[blkno%NHASH].hash_fp;buffer != &hash_head[blkno%NHASH];buffer = buffer->hash_fp) {
    if (buffer->blkno == blkno) break;
  }
  if (buffer->blkno != blkno) {
    fprintf(stderr,"Error: This buffer is not cached.\n");
    return NULL;
  }
  return buffer;
}
void brelse(struct buf_header * buffer) {
  //wakeup();
  struct buf_header * p;
  for (p = freehead.free_fp;p != &freehead;p = p->free_fp) {
    if (p == buffer) {
      fprintf(stderr,"This buuffer is already on freelist.\n");
      return;
    }
  
  }
  printf("Wakeup processes waiting for any buffer\n");
  printf("Wakeup processes waiting for buffer of blkno %d\n", buffer->blkno);
  //raise_cpu_level();
  if (((buffer->stat & STAT_VALID) != 0) & ((buffer->stat & STAT_OLD) == 0)) {
    insert_free(&freehead, buffer, 0);
    printf("Insert buffer to tail of free list\n");
  }
  else{
    insert_free(&freehead, buffer, 1);
    printf("Insert buffer to head of free list\n");
  }
  //lower_cpu_level();
  buffer->stat &= ~STAT_LOCKED;
  return;
}
void buf_state_print(struct buf_header* s) {
  int j;
  char bitchar[6] = {'O','W','K','D','V','L'};
  char stat[6] = {'-','-','-','-','-','-'};
  for (j = 0; j < 6; j++) {
    if (s->stat & 0x01 << j) stat[5-j] = bitchar[5-j];
  }
    printf("[%2d:%3d ", s->bufnum, s->blkno);
    for (j = 0; j < 6; j++) printf("%c", stat[j]);
    printf("]");
  return;
}
void printhash(int i) {
  if (i < 0 || NHASH-1 < i) {
    fprintf(stderr,"Error hash value should be 0~%d\n",NHASH-1);
  return;
  }
  struct buf_header * p;
  printf("%d: ",i);
  for (p = hash_head[i].hash_fp;p != &hash_head[i];p = p->hash_fp) {
    buf_state_print(p);
    printf(" ");
  }
  printf("\n");
}
void printfree() {
  struct buf_header * p;
  for (p = freehead.free_fp;p != &freehead;p = p->free_fp) {
    buf_state_print(p);
    printf(" ");
  }
  printf("\n");
  return;
}
void bufsetstat(int ac, char **av) {
  struct buf_header * p;
  int i;
  p = search_hash(atoi(av[1]));
  for (i = 2; i < ac; i++) {
    if (av[i][0] == 'L') p->stat |= STAT_LOCKED;
    else if (av[i][0] == 'V') p->stat |= STAT_VALID;
    else if (av[i][0] == 'D') p->stat |=STAT_DWR;
    else if (av[i][0] == 'K') p->stat |=STAT_KRDWR;
    else if (av[i][0] == 'W') p->stat |= STAT_WAITED;
    else if (av[i][0] == 'O') p->stat |=STAT_OLD;
  }
  return;
}
void bufresetstat(int ac, char **av){
  struct buf_header * p;
  int i;
  p = search_hash(atoi(av[1]));
  for (i = 2; i < ac; i++) {
    if (av[i][0] == 'L') p->stat &= ~STAT_LOCKED;
    else if (av[i][0] == 'V') p->stat &=~STAT_VALID;
    else if (av[i][0] == 'D') p->stat &=~STAT_DWR;
    else if (av[i][0] == 'K') p->stat &=~STAT_KRDWR;
    else if (av[i][0] == 'W') p->stat &=~STAT_WAITED;
    else if (av[i][0] == 'O') p->stat &=~STAT_OLD;
  }
  return;
}
