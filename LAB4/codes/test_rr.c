#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char *argv[]) {
    int pid = getpid();
    printf(1, "Starting Short RR test on PID %d\n", pid);

    volatile double x = 0; 
    volatile int k = 0;

 
    for (int i = 0; i < 2; i++) { 
        
 
        for (int j = 0; j < 5000000; j++) {
             x += 1;
             k = k + 1;
        }
 
        printf(1, "."); 
    }
    
    printf(1, "\nFinished PID %d\n", pid);
    exit();
}