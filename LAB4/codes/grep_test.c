#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  char buffer[512];  
  int len;

  if(argc != 3){
    printf(1, "Usage: grep_test <keyword> <filename>\n");
    exit();
  }

  char *keyword = argv[1];
  char *filename = argv[2];

  printf(1, "Testing grep_syscall: searching for '%s' in '%s'\n", keyword, filename);

  len = grep_syscall(keyword, filename, buffer, sizeof(buffer));

  if (len < 0) {
    printf(1, "Grep ERROR: Keyword not found or file error.\n");
  } else {
    printf(1, "Grep SUCCESS: Found line (len %d): '%s'\n", len, buffer);
  }

  exit();
}

