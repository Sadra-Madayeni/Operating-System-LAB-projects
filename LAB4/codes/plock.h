#ifndef __PLOCK_H__
#define __PLOCK_H__

#include "spinlock.h"
 
struct plock_node {
  struct proc *proc;    
  int priority;          
  struct plock_node *next;  
  int active;           
};

struct plock {
  struct spinlock lk;    
  int locked;           
  struct plock_node *head;  
  char *name;             
};

#endif