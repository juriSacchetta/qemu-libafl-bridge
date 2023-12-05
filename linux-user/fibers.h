#include "pth.h"

#ifndef QEMU_FIBERS
#define QEMU_FIBERS
#endif 

// Uncomment the line below to enable debug output
#define FIBER_DEBUG
#ifdef FIBER_DEBUG
#define FIBERS_LOG_DEBUG(fmt, ...) \
    do { qemu_log("QEMU_FIBERS (DEBUG): " fmt, ##__VA_ARGS__); } while (0)
#else
#define FIBERS_LOG_DEBUG(fmt, ...) \
    do { } while (0)
#endif

#define FIBER_INFO
#ifdef FIBER_INFO
#define FIBERS_LOG_INFO(fmt, ...) \
    do { qemu_log("QEMU_FIBERS (INFO): " fmt, ##__VA_ARGS__); } while (0)
#else
#define FIBERS_LOG_INFO(fmt, ...) \
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
void fibers_clear_all_threads(void);

int fibers_syscall_pread(int fd, void *buf, size_t nbytes, off_t offset);
int fibers_syscall_pwrite(int fd, const void *buf, size_t nbytes, off_t offset);
int fibers_syscall_pread64(int fd, void *buf, size_t nbytes, off_t offset);
int fibers_syscall_pwrite64(int fd, const void *buf, size_t nbytes, off_t offset);
int fibers_syscall_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int fibers_syscall_waitpid(pid_t pid, int *status, int options);
/*##########
# I/O OPERATIONS
###########*/

abi_long fibers_read(int fd, void *buf, size_t nbytes);
abi_long fibers_write(int fd, const void *buf, size_t nbytes);
