#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  if(argc != 2){
    printf(1, "Usage: makeduptest <filename>\n");
    exit();
  }

  printf(1, "Attempting to duplicate %s...\n", argv[1]);

  if(make_duplicate(argv[1]) == 0){
    printf(1, "SUCCESS: Created %s.copy\n", argv[1]);
  } else {
    printf(1, "ERROR: Failed to duplicate file.\n"); 
  }

  exit();
}