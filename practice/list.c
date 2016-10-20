#include <stdio.h>
#define NHASH 4
#define STAT_LOCKED 0x00000001
#define STAT_VALID 0x00000002
#define STAT_DWR 0x00000004
#define STAT_KRDWR 0x00000008
#define STAT_WAITED 0x00000010
#define STAT_OLD 0x00000020
  struct buf_header {
    int blkno;
    struct buf_header * hash_fp;
    struct buf_header * hash_bp;
    unsigned int stat;
    struct buf_header *free_fp;
    struct buf_header *free_bp;
    char * cache_data;
  };
struct buf_header hash_head[NHASH];
struct buf_header freelist;

struct buf_header * search_hash(int blkno){
  int h;
  struct buf_header *p;
  h = hash(blkno);
  for (p = hash_head[h].hash_fp;p != &hash_head[h];p = p->hash_fp) if (p->blkno == blkno) return p;
  return NULL;
}
void insert_head(struct buf_header *h, struct buf_header *p) {
  //p is insert node, h is head node.
  int ha;
  p->hash_bp = h;
  p->hash_fp = h->hash_fp;
  h->hash_fp->hash_bp = p;
  h->hash_fp = p;
}
void remove_from_hash(struct buf_header * p){
  p->hash_bp->hash_fp = p->hash_fp;
  p->hash_fp->hash_bp = p->hash_bp;
  p->hash_fp = p->hash_bp = NULL;
}

struct buf_header * getblk(int blkno) {
  return NULL;
}
void brelse(struct buf_header *buffer) {
  
}
