// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
  int cnt;
} kmem[NCPU];

void
kinit()
{
    for (int i = 0; i < NCPU; i ++) {
        initlock(&kmem[i].lock, "kmem");          
    }
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
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

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;
  push_off();
  int ci = cpuid();
  pop_off();
  acquire(&kmem[ci].lock);
  r->next = kmem[ci].freelist;
  kmem[ci].freelist = r;
  kmem[ci].cnt++;
  release(&kmem[ci].lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;
  push_off();
  int ci = cpuid();
  pop_off();
  
  acquire(&kmem[ci].lock);
  r = kmem[ci].freelist;
  if(r) {
      kmem[ci].freelist = r->next;
      kmem[ci].cnt--;
  } else {
      for (int i = 0; i < NCPU; i ++) {
          if (kmem[i].cnt > 1) {
              int tn = kmem[i].cnt / 2;
              acquire(&kmem[i].lock);
              for (int j = 0; j < tn; j ++) {
                  struct run *tr = kmem[i].freelist;
                  kmem[i].freelist = kmem[i].freelist->next;
                  kmem[i].cnt--;
                  tr->next = kmem[ci].freelist; 
                  kmem[ci].freelist = tr;
                  kmem[ci].cnt++;
              }
              release(&kmem[i].lock);
              break;
          }
      }
      if (kmem[ci].cnt == 0) {
//          panic("no memmory can be used");
      } else {
          r = kmem[ci].freelist;
          kmem[ci].freelist = r->next;
          kmem[ci].cnt--;
      }
  }
  release(&kmem[ci].lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
