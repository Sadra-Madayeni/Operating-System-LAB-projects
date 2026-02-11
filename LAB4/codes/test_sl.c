#include "types.h"
#include "user.h"

int main(void)
{
  test_init_locks();

  int pid = fork();
  if (pid < 0) {
    printf(1, "Fork failed\n");
    exit();
  }

  if (pid == 0) {
    sleep(20);
    printf(1, "Child trying to release lock acquired by parent...\n");
    test_sl_unlock(); 
    printf(1, "Child finished (Should not be reached if panic works)\n");
    exit();
  } else {
    printf(1, "Parent acquiring lock...\n");
    test_sl_lock();
    printf(1, "Parent acquired lock. Waiting for child...\n");
    wait();
    test_sl_unlock();
    printf(1, "Parent finished\n");
  }
  
  exit();
}