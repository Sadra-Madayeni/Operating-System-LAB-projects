#include "types.h"
#include "stat.h"
#include "user.h"

int
main(void)
{
  printf(1, "1. Testing getpid() system call.\n");

  int pid = getpid();
  printf(1, "2. My PID is: %d\n", pid);
  exit();
}