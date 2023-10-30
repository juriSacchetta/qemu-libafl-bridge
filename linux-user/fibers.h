#include "pth.h"

#ifndef QEMU_FIBERS
#define QEMU_FIBERS
#endif 

// Uncomment the line below to enable debug output
//#define FIBER_DEBUG
#ifdef FIBER_DEBUG
#define DEBUG_PRINT(fmt, ...) \
    fprintf(stderr, fmt, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...) \
    do { } while (0)
#endif

typedef struct {
    CPUArchState *env;
    pth_mutex_t mutex;
    pth_cond_t cond;
    pth_t thread;
    int tid;
    abi_ulong child_tidptr;
    abi_ulong parent_tidptr;
    sigset_t sigmask;
} new_thread_info;

void qemu_fibers_init(CPUArchState *env);
int register_fiber(pth_t thread, CPUArchState *cpu);

//patches for syscalls and futexes
int fibers_do_futex(int *uaddr, int op, int val, const struct timespec *timeout, target_ulong val2, int *uaddr2, int val3);
void fibers_syscall_exit(void*);
int fibers_syscall_tkill(abi_long arg1, abi_long arg2);
int fibers_syscall_tgkill(abi_long arg1, abi_long arg2, abi_long arg3);
int fibers_syscall_gettid(void);
int fibers_syscall_nanosleep(struct timespec *ts);
int fibers_syscall_clock_nanosleep(clockid_t clock_id, struct timespec *ts);

int fibers_syscall_pread(int fd, void *buf, size_t nbytes, off_t offset);
int fibers_syscall_pwrite(int fd, const void *buf, size_t nbytes, off_t offset);
int fibers_syscall_pread64(int fd, void *buf, size_t nbytes, off_t offset);
int fibers_syscall_pwrite64(int fd, const void *buf, size_t nbytes, off_t offset);

/*##########
# I/O OPERATIONS
###########*/

abi_long fibers_read(int fd, void *buf, size_t nbytes);
abi_long fibers_write(int fd, const void *buf, size_t nbytes);
