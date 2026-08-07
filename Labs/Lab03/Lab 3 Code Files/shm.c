#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"
#include "vm.h"
// #include "string.h"

struct shm_region {
  int inuse;
  int key;
  void *pa;         // physical page
  int refcnt;
};

static struct {
  struct spinlock lock;
  struct shm_region r[MAX_SHM_KEYS];
} shmtable;

void
shminit(void)
{
  initlock(&shmtable.lock, "shmtable");
  for (int i = 0; i < MAX_SHM_KEYS; i++) {
    shmtable.r[i].inuse = 0;
    shmtable.r[i].key = 0;
    shmtable.r[i].pa = 0;
    shmtable.r[i].refcnt = 0;
  }
}

// return index or -1
int
shm_create(int key)
{
  acquire(&shmtable.lock);
  // if exists, just return its index
  for (int i = 0; i < MAX_SHM_KEYS; i++) {
    if (shmtable.r[i].inuse && shmtable.r[i].key == key) {
      release(&shmtable.lock);
      return i;
    }
  }
  // find a free slot
  int idx = -1;
  for (int i = 0; i < MAX_SHM_KEYS; i++) {
    if (!shmtable.r[i].inuse) { idx = i; break; }
  }
  if (idx < 0) { release(&shmtable.lock); return -1; }

  void *pa = kalloc();
  if (pa == 0) { release(&shmtable.lock); return -1; }
  memset(pa, 0, PGSIZE);

  shmtable.r[idx].inuse = 1;
  shmtable.r[idx].key = key;
  shmtable.r[idx].pa = pa;
  shmtable.r[idx].refcnt = 0;

  release(&shmtable.lock);
  return idx;
}

// Map existing region (by key) into caller's VA space; return VA or 0 on error.
uint64
shm_get(int key)
{
  struct proc *p = myproc();
  struct shm_region *sr = 0;

  acquire(&shmtable.lock);
  for (int i = 0; i < MAX_SHM_KEYS; i++) {
    if (shmtable.r[i].inuse && shmtable.r[i].key == key) {
      sr = &shmtable.r[i];
      sr->refcnt++;
      break;
    }
  }
  release(&shmtable.lock);

  if (sr == 0) return 0; // no such region

  // find a free slot in proc's attached table
  int slot = -1;
  for (int i = 0; i < MAX_SHM_PER_PROC; i++) {
    if (!p->shm[i].used) { slot = i; break; }
  }
  if (slot < 0) {
    acquire(&shmtable.lock);
    sr->refcnt--;
    release(&shmtable.lock);
    return 0;
  }

  uint64 va = PGROUNDUP(p->sz);
  // map the *existing physical page* to the next free VA
  if (mappages(p->pagetable, va, PGSIZE, (uint64)sr->pa, PTE_R|PTE_W|PTE_U) < 0) {
    acquire(&shmtable.lock);
    sr->refcnt--;
    release(&shmtable.lock);
    return 0;
  }
  p->sz = va + PGSIZE;

  p->shm[slot].used = 1;
  p->shm[slot].key  = key;
  p->shm[slot].va   = va;

  return va;
}

int
shm_close(int key)
{
  struct proc *p = myproc();
  // find the mapping in this proc
  int slot = -1;
  uint64 va = 0;
  for (int i = 0; i < MAX_SHM_PER_PROC; i++) {
    if (p->shm[i].used && p->shm[i].key == key) {
      slot = i;
      va = p->shm[i].va;
      break;
    }
  }
  if (slot < 0) return -1;

  // unmap from this process (do not free physical page here)
  uvmunmap(p->pagetable, va, 1, 0);
  p->shm[slot].used = 0;
  p->shm[slot].key = 0;
  p->shm[slot].va = 0;

  // drop refcount, freeing the backing page when last process detaches
  acquire(&shmtable.lock);
  struct shm_region *sr = 0;
  for (int i = 0; i < MAX_SHM_KEYS; i++) {
    if (shmtable.r[i].inuse && shmtable.r[i].key == key) {
      sr = &shmtable.r[i];
      break;
    }
  }
  if (sr) {
    sr->refcnt--;
    if (sr->refcnt <= 0) {
      kfree(sr->pa);
      sr->pa = 0;
      sr->inuse = 0;
      sr->key = 0;
      sr->refcnt = 0;
    }
  }
  release(&shmtable.lock);
  return 0;
}
