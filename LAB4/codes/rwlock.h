#ifndef __RWLOCK_H__
#define __RWLOCK_H__

#include "spinlock.h"

struct rwlock {
  struct spinlock lk;
  char *name;
  int read_count;
  int write_locked;
};

#endif