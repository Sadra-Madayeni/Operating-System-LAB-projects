#include "types.h"
#include "defs.h"
#include "param.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"
#include "rwlock.h"

void
rwlock_init(struct rwlock *rw, char *name)
{
  initlock(&rw->lk, "rw spinlock");
  rw->name = name;
  rw->read_count = 0;
  rw->write_locked = 0;
}

void
rwlock_acquire_read(struct rwlock *rw)
{
  acquire(&rw->lk);
  while(rw->write_locked) {
    sleep(rw, &rw->lk);
  }
  rw->read_count++;
  release(&rw->lk);
}

void
rwlock_release_read(struct rwlock *rw)
{
  acquire(&rw->lk);
  rw->read_count--;
  if(rw->read_count == 0) {
    wakeup(rw);
  }
  release(&rw->lk);
}

void
rwlock_acquire_write(struct rwlock *rw)
{
  acquire(&rw->lk);
  while(rw->write_locked || rw->read_count > 0) {
    sleep(rw, &rw->lk);
  }
  rw->write_locked = 1;
  release(&rw->lk);
}

void
rwlock_release_write(struct rwlock *rw)
{
  acquire(&rw->lk);
  rw->write_locked = 0;
  wakeup(rw);
  release(&rw->lk);
}