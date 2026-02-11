#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char *argv[]) {
    int N = 10; 
    
    printf(1, "\n=== Throughput Test Started ===\n");
    
 
    start_measure();

 
    for(int i = 0; i < N; i++) {
        int pid = fork();
        
        if(pid < 0) {
            printf(1, "Fork failed\n");
            exit();
        }
        
        if(pid == 0) {
  
            volatile double x = 0;
            for(int j = 0; j < 100000; j++) {
                x += 1;
            }
            exit(); 
        }
    }

 
    for(int i = 0; i < N; i++) {
        wait();
    }
    
 
    printf(1, "All %d processes finished.\n", N);
    end_measure();
    
    printf(1, "=== Test Complete ===\n");
    exit();
}