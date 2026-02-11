#include "types.h"
#include "user.h"
#include "stat.h"

#define NCPU 8 
#define ITERATIONS 300  

void
worker(int i)
{
  for(int j = 0; j < ITERATIONS; j++) {
    int pid = fork();
    if(pid < 0){
      printf(1, "fork failed\n");
      break;
    }
    if(pid == 0){
      exit();
    } else {
      wait();
    }
  }
  exit();
}

int
main(int argc, char *argv[])
{
  uint scores[NCPU];

  printf(1, "Starting AGGRESSIVE Lock Contention Test on ptable.lock...\n");

  getlockstat(scores);
  printf(1, "Initial Stats (Avg Spins/Acquire):\n");
  for(int i = 0; i < NCPU; i++) {
    printf(1, "CPU %d: %d\n", i, scores[i]);
  }
 
  int children[4];
  for(int i = 0; i < 4; i++) {
    int pid = fork();
    if(pid < 0){
      printf(1, "Main fork failed\n");
      exit();
    }
    if(pid == 0){
      worker(i);
    } else {
      children[i] = pid;
    }
  }
 
  for(int i = 0; i < 4; i++) {
    wait();
  }
 
  getlockstat(scores);
  printf(1, "\nFinal Stats after contention:\n");
  for(int i = 0; i < NCPU; i++) {
    printf(1, "CPU %d: Score: %d\n", i, scores[i]);
  }

  exit();
}