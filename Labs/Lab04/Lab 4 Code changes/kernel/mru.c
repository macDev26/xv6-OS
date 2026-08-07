#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"
#include "fcntl.h"

// MRU and swap configuration
#define MAX_PAGES_PER_PROC    8     // Maximum pages per process in MRU list (reduced for testing)
#define MAX_SWAP_PAGES        (NPROC * MAX_PAGES_PER_PROC)  // Total swappable pages
#define MAX_SWAP_STORAGE      (MAX_SWAP_PAGES * PGSIZE)     // Total swap storage

// Global MRU list structure
struct {
  struct spinlock lock;
  struct mru_page *head;    // Most recently used page
  struct mru_page *tail;    // Least recently used page
  int count;                // Number of pages in the list
} mru;

// Pool of MRU page structures
static struct mru_page mru_pages[MAX_SWAP_PAGES];
static int mru_page_index = 0;

// Swap storage
static char swap_storage[MAX_SWAP_STORAGE];  // Simple swap backing store
static struct {
  struct proc *proc;
  uint64 va;
  int offset;  // Offset in swap_storage
  int used;
} swap_slots[MAX_SWAP_PAGES];
static int next_swap_slot = 0;

// Initialize the MRU system
void
mru_init(void)
{
  initlock(&mru.lock, "mru");
  mru.head = 0;
  mru.tail = 0;
  mru.count = 0;
  mru_page_index = 0;
  next_swap_slot = 0;
  
  // Initialize swap slots
  for(int i = 0; i < MAX_SWAP_PAGES; i++) {
    swap_slots[i].proc = 0;
    swap_slots[i].va = 0;
    swap_slots[i].offset = 0;
    swap_slots[i].used = 0;
  }
}

// Get a free MRU page structure
static struct mru_page*
alloc_mru_page(void)
{
  if(mru_page_index >= MAX_SWAP_PAGES)
    return 0;
  return &mru_pages[mru_page_index++];
}

// Internal helper: Update page position in MRU list (assumes lock is held)
static void
mru_update_locked(struct proc *p, uint64 va)
{
  struct mru_page *page;
  
  for(page = mru.head; page != 0; page = page->next) {
    if(page->proc == p && page->va == va) {
      // If already at head, nothing to do
      if(page == mru.head) {
        return;
      }
      
      // Remove from current position
      if(page->prev)
        page->prev->next = page->next;
      if(page->next)
        page->next->prev = page->prev;
      else
        mru.tail = page->prev;
        
      // Move to head
      page->next = mru.head;
      page->prev = 0;
      if(mru.head)
        mru.head->prev = page;
      mru.head = page;
      
      break;
    }
  }
}

// Add a page to the MRU list (most recently used position)
void
mru_add(struct proc *p, uint64 va)
{
  struct mru_page *page;
  int proc_page_count = 0;
  
  acquire(&mru.lock);
  
  // Check if page already exists in list and count pages for this process
  for(page = mru.head; page != 0; page = page->next) {
    if(page->proc == p) {
      proc_page_count++;
      if(page->va == va) {
        // Already in list, just update position
        mru_update_locked(p, va);  // Use internal version to avoid deadlock
        release(&mru.lock);
        return;
      }
    }
  }
  
  // If this process already has too many pages, evict its MRU page first
  if(proc_page_count >= MAX_PAGES_PER_PROC) {
    // Find the MRU page for this process
    struct mru_page *mru_page = 0;
    for(page = mru.head; page != 0; page = page->next) {
      if(page->proc == p) {
        mru_page = page;
        break;
      }
    }
    
    if(mru_page) {
      // Remove the MRU page for this process
      if(mru_page->prev)
        mru_page->prev->next = mru_page->next;
      else
        mru.head = mru_page->next;
        
      if(mru_page->next)
        mru_page->next->prev = mru_page->prev;
      else
        mru.tail = mru_page->prev;
        
      mru.count--;
      
      // Swap out the evicted page
      release(&mru.lock);
      swap_out(mru_page->proc, mru_page->va);
      acquire(&mru.lock);
    }
  }
  
  // Allocate new MRU page structure
  page = alloc_mru_page();
  if(page == 0) {
    release(&mru.lock);
    return; // Out of MRU page structures
  }
  
  page->proc = p;
  page->va = va;
  page->next = mru.head;
  page->prev = 0;
  
  if(mru.head)
    mru.head->prev = page;
  mru.head = page;
  
  if(mru.tail == 0)
    mru.tail = page;
    
  mru.count++;
  
  release(&mru.lock);
}

// Remove a page from the MRU list
void
mru_remove(struct proc *p, uint64 va)
{
  struct mru_page *page;
  
  acquire(&mru.lock);
  
  for(page = mru.head; page != 0; page = page->next) {
    if(page->proc == p && page->va == va) {
      if(page->prev)
        page->prev->next = page->next;
      else
        mru.head = page->next;
        
      if(page->next)
        page->next->prev = page->prev;
      else
        mru.tail = page->prev;
        
      mru.count--;
      break;
    }
  }
  
  release(&mru.lock);
}

// Update a page's position in the MRU list (move to head)
void
mru_update(struct proc *p, uint64 va)
{
  acquire(&mru.lock);
  mru_update_locked(p, va);
  release(&mru.lock);
}

// Evict the MOST recently used page (head of list)
struct mru_page*
mru_evict(void)
{
  struct mru_page *page;
  
  acquire(&mru.lock);
  
  // Check if list is empty
  if(mru.head == 0) {
    release(&mru.lock);
    return 0;
  }
  
  page = mru.head;  // Evict from head (MRU)
  
  // Remove from head
  mru.head = page->next;
  if(mru.head)
    mru.head->prev = 0;
  else
    mru.tail = 0;
    
  mru.count--;
  
  release(&mru.lock);
  return page;
}

// Dump the MRU list to console
void
mru_dump(void)
{
  struct mru_page *page;
  int i = 0;
  
  acquire(&mru.lock);
  
  printf("MRU List (Most Recently Used first):\n");
  if(mru.count == 0) {
    printf("  (empty)\n");
  } else {
    for(page = mru.head; page != 0; page = page->next) {
      printf("  %d: PID=%d VA=0x%lx\n", i++, page->proc->pid, page->va);
    }
  }
  printf("Total pages in MRU list: %d (max per process: %d)\n", mru.count, MAX_PAGES_PER_PROC);
  
  release(&mru.lock);
}

// Swap out a page to disk
int
swap_out(struct proc *p, uint64 va)
{
  pte_t *pte;
  uint64 pa;
  
  // Find the page table entry
  pte = walk(p->pagetable, va, 0);
  if(pte == 0 || (*pte & PTE_V) == 0)
    return -1;
    
  pa = PTE2PA(*pte);
  
  // Find or allocate swap slot
  int slot = -1;
  for(int i = 0; i < next_swap_slot; i++) {
    if(swap_slots[i].proc == p && swap_slots[i].va == va) {
      slot = i;
      break;
    }
  }
  
  if(slot == -1) {
    if(next_swap_slot >= NPROC * 50)
      return -1;
    slot = next_swap_slot++;
    swap_slots[slot].proc = p;
    swap_slots[slot].va = va;
    swap_slots[slot].offset = slot * PGSIZE;
  }
  
  swap_slots[slot].used = 1;
  
  // Copy page data to swap storage (FIXED: Actually save the data!)
  memmove(&swap_storage[swap_slots[slot].offset], (void*)pa, PGSIZE);
  
  // Mark page as swapped out in page table
  *pte = (*pte & ~PTE_V) | PTE_S; // Clear valid bit, set swapped bit
  
  // Free the physical page
  kfree((void*)pa);
  
  // Remove from MRU list
  mru_remove(p, va);
  
  p->swap_outs++;
  
  return 0;
}

// Swap in a page from disk
int
swap_in(struct proc *p, uint64 va)
{
  pte_t *pte;
  uint64 pa;
  
  // Find the page table entry
  pte = walk(p->pagetable, va, 0);
  if(pte == 0 || (*pte & PTE_S) == 0)
    return -1;
  
  // Find swap slot
  int slot = -1;
  for(int i = 0; i < next_swap_slot; i++) {
    if(swap_slots[i].proc == p && swap_slots[i].va == va && swap_slots[i].used) {
      slot = i;
      break;
    }
  }
  
  if(slot == -1)
    return -1;
    
  // Allocate physical page
  pa = (uint64)kalloc();
  if(pa == 0)
    return -1;
    
  // Restore page data from swap storage (FIXED: Actually restore the data!)
  memmove((void*)pa, &swap_storage[swap_slots[slot].offset], PGSIZE);
  
  // Update page table entry
  *pte = PA2PTE(pa) | PTE_FLAGS(*pte) | PTE_V;
  *pte &= ~PTE_S; // Clear swapped bit
  
  // Add to MRU list
  mru_add(p, va);
  
  p->swap_ins++;
  
  return 0;
}

// Handle page fault for demand paging
int
handle_page_fault(struct proc *p, uint64 va)
{
  pte_t *pte;
  
  // Check if this is a valid virtual address for the process
  if(va >= p->sz) {
    return -1;
  }
    
  va = PGROUNDDOWN(va); // Align to page boundary
    
  // Find page table entry
  pte = walk(p->pagetable, va, 1); // Create PTE if it doesn't exist
  if(pte == 0) {
    return -1;
  }
  
  // If page is already valid, this shouldn't be a page fault for us to handle
  if(*pte & PTE_V) {
    return -1; // Let other handlers deal with it
  }
    
  p->page_faults++;
  
  // Check if page was swapped out
  if(*pte & PTE_S) {
    // Page is swapped out, swap it back in
    if(swap_in(p, va) < 0) {
      return -1;
    }
  } else {
    // Page not allocated yet (lazy allocation)
    uint64 pa = (uint64)kalloc();
    if(pa == 0) {
      // Out of memory, need to evict a page
      
      // Only try eviction if we have pages to evict
      if(mru.count == 0) {
        return -1;
      }
      
      struct mru_page *victim = mru_evict();
      if(victim == 0) {
        return -1;
      }
        
      // Swap out the victim page
      if(swap_out(victim->proc, victim->va) < 0) {
        return -1;
      }
        
      // Try to allocate again
      pa = (uint64)kalloc();
      if(pa == 0) {
        return -1;
      }
    }
    
    memset((void*)pa, 0, PGSIZE);
    
    // Set the page table entry directly instead of using mappages
    *pte = PA2PTE(pa) | PTE_W | PTE_X | PTE_R | PTE_U | PTE_V;
    
    // Add to MRU list
    mru_add(p, va);
  }
  
  return 0;
}