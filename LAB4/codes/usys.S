#include "syscall.h"
#include "traps.h"

#define SYSCALL(name) \
  .globl name; \
  name: \
    movl $SYS_ ## name, %eax; \
    int $T_SYSCALL; \
    ret

SYSCALL(fork)
SYSCALL(exit)
SYSCALL(wait)
SYSCALL(pipe)
SYSCALL(read)
SYSCALL(write)
SYSCALL(close)
SYSCALL(kill)
SYSCALL(exec)
SYSCALL(open)
SYSCALL(mknod)
SYSCALL(unlink)
SYSCALL(fstat)
SYSCALL(link)
SYSCALL(mkdir)
SYSCALL(chdir)
SYSCALL(dup)
SYSCALL(getpid)
SYSCALL(sbrk)
SYSCALL(sleep)
SYSCALL(uptime)
SYSCALL(make_duplicate)
SYSCALL(show_process_family)
SYSCALL(grep_syscall)
SYSCALL(set_priority_syscall)
SYSCALL(start_measure)
SYSCALL(end_measure)
SYSCALL(print_info)
SYSCALL(test_init_locks)
SYSCALL(test_sl_lock)
SYSCALL(test_sl_unlock)
SYSCALL(test_rw_read_lock)
SYSCALL(test_rw_read_unlock)
SYSCALL(test_rw_write_lock)
SYSCALL(test_rw_write_unlock)
SYSCALL(getlockstat)

SYSCALL(plock_acquire)
SYSCALL(plock_release)


SYSCALL(rw_reader_enter)
SYSCALL(rw_reader_exit)
SYSCALL(rw_writer_enter)
SYSCALL(rw_writer_exit)