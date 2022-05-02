// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

const uint BKS = 13;

struct {
  struct spinlock lock;
  struct buf buf[NBUF];
} bcache;

struct {
  struct buf *entry; 
  struct spinlock lock;
} ht[13];

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    initsleeplock(&b->lock, "buffer");
    b->refcnt = 0;
  }
  
  for (int i= 0; i < BKS; i ++) {
      initlock(&ht[i].lock, "bcache");
  }
}

void 
bremove(uint idx, uint dev, uint no)
{
    struct buf *pre = 0, *b;
    for (b = ht[idx].entry; b; b = b->next) {
        if (b->dev == dev && b->blockno == no) {
            if (pre) {
                pre->next = b->next;
            } else {
                ht[idx].entry = b->next;
            }
            b->next = 0;
            return;
        }
        pre = b;
    }
}

void 
binsert(uint idx, struct buf *b) 
{
    b->next = ht[idx].entry;
    ht[idx].entry = b;
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  uint hti = blockno % BKS;
  
  struct buf *b;

  acquire(&ht[hti].lock);

  // Is the block already cached?
  for(b = ht[hti].entry; b; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&ht[hti].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  acquire(&bcache.lock);
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    if(b->refcnt == 0) {
      bremove(b->blockno % BKS, b->dev, b->blockno);
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock);
      binsert(hti, b);
      release(&ht[hti].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");
  b->refcnt--;
  releasesleep(&b->lock);
}

void
bpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt++;
  release(&bcache.lock);
}

void
bunpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt--;
  release(&bcache.lock);
}


