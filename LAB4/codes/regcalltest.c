#include "types.h"
#include "stat.h"
#include "user.h"
#include "syscall.h" 

int
main(void)
{
  int a = 5;
  int b = 3;
  int result;

  printf(1, "Testing register-based system call with a=%d, b=%d\n", a, b);

__asm__ __volatile__(
        "movl %1, %%eax\n\t" 
        "movl %2, %%ebx\n\t"   
        "movl %3, %%ecx\n\t"   
        "int $64\n\t"        
        "movl %%eax, %0"     
        : "=r" (result)      
        : "i" (SYS_simple_arithmetic_syscall), 
          "r" (a),        
          "r" (b)         
        : "%eax", "%ebx", "%ecx" 
      );

  printf(1, "System call returned: %d\n", result);



  exit();
}