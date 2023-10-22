#include "pth.h"

void qemu_fibers_init(CPUArchState *env);
int qemu_fibers_spawn(void *info);

//patches for syscalls and futexes
int fibers_do_futex(int *uaddr, int op, int val, const struct timespec *timeout, int *uaddr2, int val3);
void fibers_syscall_exit(void*);
int fibers_syscall_tkill(abi_long arg1, abi_long arg2);
int fibers_syscall_tgkill(abi_long arg1, abi_long arg2, abi_long arg3);
int fibers_syscall_gettid(void);
int fibers_syscall_nanosleep(struct timespec *ts);
int fibers_syscall_clock_nanosleep(clockid_t clock_id, struct timespec *ts);
int fibers_syscall_clone(CPUState *cpu, target_ulong clone_flags, target_ulong newsp, target_ulong parent_tidptr, target_ulong child_tidptr, target_ulong tls);


/*##########
# I/O OPERATIONS
###########*/

abi_long fibers_read(int fd, void *buf, size_t nbytes);
abi_long fibers_write(int fd, const void *buf, size_t nbytes);
