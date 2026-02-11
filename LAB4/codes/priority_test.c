#include "types.h"
#include "stat.h"
#include "user.h"

void cpu_heavy_loop(const char* name)
{
  long i;
  for (i = 0; i < 400000000; i++) {
    if (i % 100000000 == 0)
        printf(1, "Process %s still running...\n", name);
  }
  printf(1, "--- Process %s FINISHED --- \n", name);
}

int
main(void)
{
  int pid1, pid2;

  printf(1, "Starting priority test...\n");

  pid1 = fork();  
  if (pid1 == 0) {
    cpu_heavy_loop("Child 1 (Low Priority)");
    exit();
  }
  
  pid2 = fork(); 
  if (pid2 == 0) {
    cpu_heavy_loop("Child 2 (High Priority)");
    exit();
  }
  
  set_priority_syscall(pid1, 2); 
  set_priority_syscall(pid2, 0);  
  printf(1, "Parent waiting for children...\n");
  wait();
  wait();
  printf(1, "Priority test finished.\n");
  exit();
}