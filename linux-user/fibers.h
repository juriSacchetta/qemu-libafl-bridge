#include <ucontext.h>

void qemu_fibers_may_switch(void);
void qemu_fibers_init(CPUArchState *env);
int qemu_fibers_create(void *info);

//patches for syscalls and futexes
int fibers_futex_wait(CPUState *cpu, target_ulong uaddr, int op, int val, struct timespec *pts);
int fibers_futex_requeue(CPUState *cpu, int base_op, int val, int val3, target_ulong uaddr, target_ulong uaddr2, target_ulong timeout);

void fibers_syscall_exit(void);
int fibers_syscall_tkill(abi_long arg1, abi_long arg2);
int fibers_syscall_tgkill(abi_long arg1, abi_long arg2, abi_long arg3);
int fibers_syscall_gettid(void);
int fibers_syscall_clock_nanosleep(clockid_t clock_id, struct timespec *ts);
int fibers_syscall_clone(CPUState *cpu, target_ulong clone_flags, target_ulong newsp, target_ulong parent_tidptr, target_ulong child_tidptr, target_ulong tls);