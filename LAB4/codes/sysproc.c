#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h" 
#include "sleeplock.h"
#include "rwlock.h"
#include "plock.h"

extern struct ptable ptable;
extern struct spinlock tickslock;
extern uint ticks;
extern struct plock global_plock;
extern struct rwlock global_rwlock;
extern int start_measure(void);
extern int end_measure(void);
extern int print_info(void);

struct sleeplock test_sl;
struct rwlock test_rw;

int
sys_test_init_locks(void)
{
  initsleeplock(&test_sl, "test_sleep");
  rwlock_init(&test_rw, "test_rw");
  return 0;
}

int
sys_test_sl_lock(void)
{
  acquiresleep(&test_sl);
  return 0;
}

int
sys_test_sl_unlock(void)
{
  releasesleep(&test_sl);
  return 0;
}

int
sys_test_rw_read_lock(void)
{
  rwlock_acquire_read(&test_rw);
  return 0;
}

int
sys_test_rw_read_unlock(void)
{
  rwlock_release_read(&test_rw);
  return 0;
}

int
sys_test_rw_write_lock(void)
{
  rwlock_acquire_write(&test_rw);
  return 0;
}

int
sys_test_rw_write_unlock(void)
{
  rwlock_release_write(&test_rw);
  return 0;
}

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0; 
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int
sys_simple_arithmetic_syscall(void)
{
  int a, b;
  int result;
  struct proc *curproc = myproc();

  a = curproc->tf->ebx; 
  b = curproc->tf->ecx;  

  result = (a + b) * (a - b); 

  cprintf("Calc: (%d+%d)*(%d-%d)=%d\n", a, b, a, b, result);  
  return result;
}

int
sys_show_process_family(void) 
{
  struct proc *p = 0;  
  struct proc *parent;
  int found_child = 0;
  int found_sibling = 0;
  int pid;  

  if(argint(0, &pid) < 0)
    return -1;
  acquire(&ptable.lock);
  for(struct proc *np = ptable.proc; np < &ptable.proc[NPROC]; np++){
    if(np->pid == pid){
      p = np;
      break;
    }
  }

  if(p == 0){
    release(&ptable.lock);
    cprintf("Process with PID %d not found.\n", pid);
    return -1;
  }

  parent = p->parent;
  if(parent){
    cprintf("My id: %d, My parent id: %d\n", p->pid, parent->pid);
  } else {
    cprintf("My id: %d, I have no parent.\n", p->pid);
  }

  cprintf("Children of process %d:\n", p->pid);
  for(struct proc *np = ptable.proc; np < &ptable.proc[NPROC]; np++){
    if(np->parent == p){ 
      cprintf("Child pid: %d\n", np->pid);
      found_child = 1;
    }
  }
  if(!found_child)
    cprintf("No children found.\n");

  cprintf("Siblings of process %d:\n", p->pid);
  if(!parent){
    cprintf("No siblings (process has no parent).\n");
  } else {
    for(struct proc *np = ptable.proc; np < &ptable.proc[NPROC]; np++){
      if(np->parent == parent && np != p){  
        cprintf("Sibling pid: %d\n", np->pid);
        found_sibling = 1;
      }
    }   
  }
  if(parent && !found_sibling)
    cprintf("No siblings found.\n");

  release(&ptable.lock);  
  return 0;  
}

int
sys_set_priority_syscall(void)
{
  int pid, priority;
  struct proc *p;

  if (argint(0, &pid) < 0 || argint(1, &priority) < 0)
    return -1;

  if (priority < 0 || priority > 2)
    return -1;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->priority = priority;  
      release(&ptable.lock);
      return 0;  
    }
  }

  release(&ptable.lock);
  return -1;
}

int
sys_start_measure(void)
{
  return start_measure();
}

int
sys_end_measure(void)
{
  return end_measure();
}

int
sys_print_info(void)
{
  return print_info();
}

int
sys_getlockstat(void)
{
  uint *user_scores; 
   
  if(argptr(0, (void*)&user_scores, sizeof(uint)*NCPU) < 0)
    return -1;

  uint kscores[NCPU];  

  for(int i = 0; i < NCPU; i++){
 
    
    uint total = ptable.lock.total_spins[i];
    uint acq = ptable.lock.acq_count[i];
    
    if (acq == 0)
       kscores[i] = 0;
    else
       kscores[i] = total / acq;  
  }
 
  if(copyout(myproc()->pgdir, (uint)user_scores, (void*)kscores, sizeof(uint)*NCPU) < 0)
    return -1;

  return 0;
}

int
sys_plock_acquire(void)
{
  int priority;
 
  if(argint(0, &priority) < 0)
    return -1;
    
  plock_acquire(&global_plock, priority);
  return 0;
}

int
sys_plock_release(void)
{
  plock_release(&global_plock);
  return 0;
}


int sys_rw_reader_enter(void) {
  rwlock_acquire_read(&global_rwlock);
  return 0;
}

 
int sys_rw_reader_exit(void) {
  rwlock_release_read(&global_rwlock);
  return 0;
}

 
int sys_rw_writer_enter(void) {
  rwlock_acquire_write(&global_rwlock);
  return 0;
}

 
int sys_rw_writer_exit(void) {
  rwlock_release_write(&global_rwlock);
  return 0;
}