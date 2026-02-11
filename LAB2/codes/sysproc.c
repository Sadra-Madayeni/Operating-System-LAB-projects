#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h" 

extern struct ptable ptable;
extern struct spinlock tickslock;
extern uint ticks;


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