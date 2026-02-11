#include "types.h"
#include "stat.h"
#include "user.h"

 
void heavy_work() {
    volatile double x = 0;
 
    for(volatile int k = 0; k < 500000; k++) {
        x += 1;
        x = x * 1.000001;
    }
}

int main(int argc, char *argv[]) {
    int n = 3; 
    printf(1, "\n=== Starting FCFS Test on P-core ===\n");
    printf(1, "Expectation: Child 1 finishes, THEN Child 2, THEN Child 3.\n\n");

    for(int i = 1; i <= n; i++) {
        int pid = fork();
        
        if(pid < 0) {
            printf(1, "Fork failed\n");
            exit();
        }
        
        if(pid == 0) {
            int my_pid = getpid();
            printf(1, "Child %d (Order #%d) Created.\n", my_pid, i);
            
 
            for(int j=0; j<2; j++) {
                heavy_work(); 
            }
            
            printf(1, "Child %d (Order #%d) FINISHED.\n", my_pid, i);
            exit();
        } 
        else {
            sleep(10); 
        }
    }

    for(int i = 0; i < n; i++) {
        wait();
    }
    
    printf(1, "\n=== FCFS Test Complete ===\n");
    exit();
}