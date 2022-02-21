// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

int cnt[32768];
void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

uint64 getpi(uint64 pa) {
    if (pa-KERNBASE<0) {
        printf("getpi pa=%p\n", pa);
    } else if ((pa-KERNBASE) / 4096 < 0) {
        printf("getpi2 pa=%p\n", pa);
    }
    return (pa-KERNBASE) / 4096;
}

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  memset(cnt, 0, sizeof(cnt));
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  int t = --cnt[getpi((uint64)pa)];
  if (t > 0) {
//      printf("cnt=%d\n", t);
      return;
  }
  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r) {
    memset((char*)r, 5, PGSIZE); // fill with junk
    int pi = getpi((uint64)r);
    cnt[pi] = 1;
  }
  return (void*)r;
}

void decrrefcnt(uint64 pa) {
//    printf("decr old%d\n", cnt[getpi(pa)]);
    cnt[getpi(pa)]--;
//    printf("decr new%d\n", cnt[getpi(pa)]);
}

void incrrefcnt(uint64 pa) {
    cnt[getpi(pa)]++;
}