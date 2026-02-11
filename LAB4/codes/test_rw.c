#include "types.h"
#include "user.h"

void reader(int id) {
  printf(1, "Reader %d waiting...\n", id);
  test_rw_read_lock();
  printf(1, "Reader %d entered critical section.\n", id);
  sleep(50);
  printf(1, "Reader %d leaving...\n", id);
  test_rw_read_unlock();
}

void writer(int id) {
  printf(1, "Writer %d waiting...\n", id);
  test_rw_write_lock();
  printf(1, "Writer %d entered critical section (Exclusive).\n", id);
  sleep(50);
  printf(1, "Writer %d leaving...\n", id);
  test_rw_write_unlock();
}

int main(void)
{
  test_init_locks();

  int i;
  for(i = 0; i < 3; i++) {
    if(fork() == 0) {
      reader(i);
      exit();
    }
  }

  if(fork() == 0) {
    writer(100);
    exit();
  }

  for(i = 0; i < 3; i++) {
    if(fork() == 0) {
      reader(i+10);
      exit();
    }
  }
  
  for(i = 0; i < 7; i++) wait();
  exit();
}