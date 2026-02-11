#include "types.h"
#include "user.h"

void worker(int priority) {
    printf(1, "Child (pid: %d) requesting lock with priority %d\n", getpid(), priority);
    plock_acquire(priority);
    printf(1, "Child (pid: %d) ACQUIRED lock (Priority %d)\n", getpid(), priority);
    sleep(10); // کمی کار انجام دهد
    plock_release();
    exit();
}

int main() {
    printf(1, "Starting Priority Lock Test...\n");
 
    plock_acquire(0);
    printf(1, "Parent acquired lock. Creating children...\n");
 
    if(fork() == 0) worker(10);
    sleep(5); 
 
    if(fork() == 0) worker(50);
    sleep(5);
 
    if(fork() == 0) worker(30);
    sleep(5);

    printf(1, "Parent releasing lock. Winner should be Priority 50...\n");
    plock_release();
 
    wait(); wait(); wait();
    exit();
}