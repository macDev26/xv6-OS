// top of file
#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"

struct mailbox {
  struct spinlock lock;
  int inuse;
  int key;
  int buf[MBOX_CAP];
  int head;   // pop from head
  int tail;   // push at tail
  int count;
  // channels for sleep/wakeup: use pointers (stable addresses)
  void *not_empty;
  void *not_full;
};


static struct {
  struct spinlock lock;
  struct mailbox b[MBOX_MAX];
} mtable;

void
mboxinit(void)
{
  initlock(&mtable.lock, "mbtable");
  for (int i = 0; i < MBOX_MAX; i++) {
    struct mailbox *m = &mtable.b[i];
    initlock(&m->lock, "mbox");
    m->inuse = 0;
    m->key = 0;
    m->head = m->tail = m->count = 0;
    // Use stable addresses inside the mailbox struct as channels
    m->not_empty = (void *)&m->head;   // distinct pointer inside this mailbox
    m->not_full  = (void *)&m->tail;   // distinct pointer inside this mailbox
  }
}


// returns mbox id or -1
int
mbox_create(int key)
{
  acquire(&mtable.lock);
  for (int i = 0; i < MBOX_MAX; i++) {
    if (mtable.b[i].inuse && mtable.b[i].key == key) {
      release(&mtable.lock);
      return i; // already exists
    }
  }
  int idx = -1;
  for (int i = 0; i < MBOX_MAX; i++) {
    if (!mtable.b[i].inuse) { idx = i; break; }
  }
  if (idx < 0) { release(&mtable.lock); return -1; }
  struct mailbox *m = &mtable.b[idx];
  m->inuse = 1;
  m->key = key;
  m->head = m->tail = m->count = 0;
  release(&mtable.lock);
  return idx;
}

int
mbox_send_k(int id, int msg)
{
  if (id < 0 || id >= MBOX_MAX) return -1;
  struct mailbox *m = &mtable.b[id];
  if (!m->inuse) return -1;

  acquire(&m->lock);
  while (m->count == MBOX_CAP) {
    sleep(&m->not_full, &m->lock);
  }
  m->buf[m->tail] = msg;
  m->tail = (m->tail + 1) % MBOX_CAP;
  m->count++;
  wakeup(&m->not_empty);
  release(&m->lock);
  return 0;
}

int
mbox_recv_k(int id, int *out)
{
  if (id < 0 || id >= MBOX_MAX) return -1;
  struct mailbox *m = &mtable.b[id];
  if (!m->inuse) return -1;

  acquire(&m->lock);
  while (m->count == 0) {
    sleep(&m->not_empty, &m->lock);
  }
  int v = m->buf[m->head];
  m->head = (m->head + 1) % MBOX_CAP;
  m->count--;
  wakeup(&m->not_full);
  release(&m->lock);

  *out = v;
  return 0;
}
