#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  if(argc != 2){
    printf(1, "Usage: pidtest <pid>\n");
    exit();
  }

  int pid = atoi(argv[1]); 

  printf(1, "--- Calling show_process_family(%d) ---\n", pid);

  if(show_process_family(pid) < 0){
    printf(1, "--- Error: Process %d not found ---\n", pid);
  } else {
  }

  exit();
}