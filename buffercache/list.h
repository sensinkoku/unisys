#include <stdio.h>
#define NHASH 4
#define STAT_LOCKED 0x00000001
#define STAT_VALID  0x00000002
#define STAT_DWR    0x00000004
#define STAT_KRDWR  0x00000008
#define STAT_WAITED 0x00000010
#define STAT_OLD    0x00000020
//functions
//struct buf_header * search_free(int blkno);

//struct
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
void brelse(struct buf_header *buffer);
void printhash(int i);
void printfree();
void buf_state_print(struct buf_header*);
struct buf_header * search_hash(int blkno);
//global v
extern struct buf_header hash_head[NHASH];
extern struct buf_header freehead;
extern struct buf_header buffers[12];
//extern struct buf_header 

struct buf_header * search_hash(int blkno){
  int h;
  struct buf_header *p;
  h = blkno % NHASH;
  for (p = hash_head[h].hash_fp;p != &hash_head[h];p = p->hash_fp) if (p->blkno == blkno) return p;
  return NULL;
}
/*struct buf_header * search_free(int blkno){
  int h;
  struct buf_header *p;
  for (p = freehead.hash_fp;p != &freehead;p = p->hash_fp){
    if(p->blkno == blkno) return p;
  }
  return NULL;
}*/
void insert_head(struct buf_header *h, struct buf_header *p) {
  //p is insert node, h is head node.
  int ha;
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
  p->free_fp = p->free_bp = NULL;
}
void remove_from_hash(struct buf_header * p){
  p->hash_bp->hash_fp = p->hash_fp;
  p->hash_fp->hash_bp = p->hash_bp;
  p->hash_fp = p->hash_bp = NULL;
}
struct buf_header * getblk(int blkno) {
  struct buf_header * p;
  struct buf_header * fp;
  fp = freehead.free_fp;
  while(fp != &freehead){
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
	printf("scenario4\n");
        	//sleep();
        	printf("Process goes to sleep\n");
        	return NULL;
      }
      if(fp->stat & STAT_DWR) {
      	//sinario3
	printf("scenario3\n");
      	//write asynchronous
        fp = fp->free_fp;
      	continue;
      }
      //sinario2
      printf("scenario2\n");
      remove_from_free(fp);
      remove_from_hash(fp);
      insert_head(&hash_head[blkno % NHASH],fp);
      fp->blkno = blkno;
      return fp;
    }
  }
  return NULL;
}
void brelse(struct buf_header *buffer) {
  //wakeup();
  printf("Wakeup processes waiting for any buffer\n");
  printf("Wakeup processes waiting for buffer of blkno $d\n", buffer->blkno);
  //raise_cpu_level();
  if ((buffer->stat & STAT_VALID) & ~(buffer->stat & STAT_OLD))
    insert_free(&freehead, buffer, 0);
  else
    insert_free(&freehead, buffer, 1);
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
}
