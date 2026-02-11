#include "types.h"
#include "user.h"

// تست خواننده
void reader(int id) {
    printf(1, "[Reader %d] Requesting lock...\n", id);
    rw_reader_enter();  
 
    printf(1, "[Reader %d] ENTERED. Reading...\n", id);
    sleep(100);  
    printf(1, "[Reader %d] Exiting.\n", id);
    
    rw_reader_exit(); 
    exit();
}

// تست نویسنده
void writer(int id) {
    printf(1, "[Writer %d] Requesting lock...\n", id);
    rw_writer_enter();  
  
    printf(1, ">>> [Writer %d] WRITING (Exclusive) <<<\n", id);
    sleep(200);  
    printf(1, ">>> [Writer %d] DONE.\n", id);
    
    rw_writer_exit();  
    exit();
}

int main() {
    printf(1, "Starting RW-Lock Test...\n");
 
    for(int i=1; i<=3; i++){
        if(fork() == 0) reader(i);
    }

    sleep(20); 

   
    if(fork() == 0) writer(99);
 
    for(int i=0; i<4; i++) wait();
    
    printf(1, "Test Finished.\n");
    exit();
}