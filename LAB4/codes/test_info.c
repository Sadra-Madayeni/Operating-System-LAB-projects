#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char *argv[]) {
    printf(1, "\n=== Process Info Test ===\n");
    
 
    print_info();
    
 
    int pid = fork();
    if(pid == 0) {
        printf(1, "\n[Child Process Reporting]\n");
        print_info();
        exit();
    }
    
    wait();
    printf(1, "\n=== Test Complete ===\n");
    exit();
}