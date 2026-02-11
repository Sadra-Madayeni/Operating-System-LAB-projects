#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "plock.h"
 
struct plock_node node_pool[NPROC];
struct spinlock pool_lock;
 
struct plock_node*
alloc_node(void)
{
  acquire(&pool_lock);
  for(int i = 0; i < NPROC; i++){
    if(node_pool[i].active == 0){
      node_pool[i].active = 1;
      release(&pool_lock);
      return &node_pool[i];
    }
  }
  release(&pool_lock);
  panic("plock: out of nodes"); 
  return 0;
}

void
free_node(struct plock_node *n)
{
  acquire(&pool_lock);
  n->active = 0;
  n->proc = 0;
  n->next = 0;
  release(&pool_lock);
}

 
void
plock_init(struct plock *pl, char *name)
{
  initlock(&pl->lk, "plock_lk");  
  initlock(&pool_lock, "pool_lk");  
  pl->name = name;
  pl->locked = 0;
  pl->head = 0;
  
 
  for(int i = 0; i < NPROC; i++)
    node_pool[i].active = 0;
}

 
void
plock_acquire(struct plock *pl, int priority)
{
  acquire(&pl->lk);  

  if(pl->locked == 0) {
 
    pl->locked = 1;
    release(&pl->lk);
  } else {
 
    struct plock_node *n = alloc_node();
    n->proc = myproc();
    n->priority = priority;
  
    n->next = pl->head;
    pl->head = n;
 
    sleep(n, &pl->lk); 
 
    
    free_node(n); 
    release(&pl->lk);
  }
}

 
void
plock_release(struct plock *pl)
{
  acquire(&pl->lk);

  if(pl->head == 0){
 
    pl->locked = 0;
  } else {
 
    struct plock_node *max_node = 0;
    struct plock_node *prev = 0;
    
    struct plock_node *curr = pl->head;
    struct plock_node *prev_curr = 0;
    
    struct plock_node *max_prev = 0;
    int max_prio = -1;  

   
    while(curr != 0){
      if(curr->priority > max_prio){
        max_prio = curr->priority;
        max_node = curr;
        max_prev = prev_curr;
      }
      prev_curr = curr;
      curr = curr->next;
    }

 
    if(max_prev == 0){
      pl->head = max_node->next;  
    } else {
      max_prev->next = max_node->next; 
    }

 
    
    wakeup(max_node);  
  }

  release(&pl->lk);
}