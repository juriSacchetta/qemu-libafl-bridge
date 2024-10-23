#pragma once

#include "pth/pth.h"
#include "qemu.h"
#include "qemu/osdep.h"
#include "src/fibers-types.h"
#include "src/fibers-utils.h"

typedef struct
{
    CPUArchState *env;
    pth_mutex_t mutex;
    pth_cond_t cond;
    pth_t thread;
    int tid;
    abi_ulong child_tidptr;
    abi_ulong parent_tidptr;
    sigset_t sigmask;
} new_thread_info;

void fibers_init(CPUState *cpu);
void fibers_fork_end(bool child);
qemu_fiber *fibers_spawn(int tid, CPUArchState *cpu, void *(*func)(void *), void *arg);
void fiber_exit(bool continue_execution);

#ifdef AS_LIB
int libafl_qemu_run(void);
void fibers_save_stopped_thread(CPUArchState *cpu);
void fibers_restore_thread(int tid, CPUArchState *s);
int fibers_get_tid_by_cpu(CPUArchState *cpu);
#endif

void fibers_call_scheduler(void);

DECLARE_FIBER_SYSCALL(int, accept4, int fd, struct sockaddr *addr, socklen_t *len, int flags)
DECLARE_FIBER_SYSCALL(int, clock_nanosleep, const clockid_t clock, int flags, const struct timespec * req, struct timespec * rem)
DECLARE_FIBER_SYSCALL(int, connect, int sockfd, const struct sockaddr *addr, socklen_t addrlen)
DECLARE_FIBER_SYSCALL(int, futex, int *uaddr, int op, int val, const struct timespec *timeout, int *uaddr2, int val3)
DECLARE_FIBER_SYSCALL(int, gettid, void)
DECLARE_FIBER_SYSCALL(int, nanosleep, const struct timespec *req, struct timespec *rem)
DECLARE_FIBER_SYSCALL(int, ppoll, struct pollfd *fds, unsigned int nfds, const struct timespec *timeout_ts, const sigset_t *sigmask)
DECLARE_FIBER_SYSCALL(int, prctl, int option, abi_ulong arg2, abi_ulong arg3, abi_ulong arg4, abi_ulong arg5)
DECLARE_FIBER_SYSCALL(ssize_t, pread64, int fd, void *buf, size_t nbytes, off_t offset)
DECLARE_FIBER_SYSCALL(ssize_t, pwrite64, int fd, const void *buf, size_t nbytes, off_t offset)
DECLARE_FIBER_SYSCALL(ssize_t, read, int fd, void *buf, size_t nbytes)
DECLARE_FIBER_SYSCALL(ssize_t, readv, int fd, const struct iovec *iov, int iovcnt)
DECLARE_FIBER_SYSCALL(ssize_t, recvfrom, int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen)
DECLARE_FIBER_SYSCALL(ssize_t, sendto, int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen)
DECLARE_FIBER_SYSCALL(int, tkill, int tid, int sig)
DECLARE_FIBER_SYSCALL(int, tgkill, int arg1, int arg2, int arg3)
DECLARE_FIBER_SYSCALL(pid_t, wait4, pid_t pid, int *status, int options, struct rusage *rusage)
DECLARE_FIBER_SYSCALL(int, waitpid, pid_t pid, int *status, int options)
DECLARE_FIBER_SYSCALL(ssize_t, write, int fd, const void *buf, size_t nbytes)
DECLARE_FIBER_SYSCALL(ssize_t, writev, int fd, const struct iovec *iov, int iovcnt)