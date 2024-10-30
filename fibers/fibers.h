#pragma once

#include "pth/pth.h"
#include "qemu/osdep.h"
#include "exec/user/abitypes.h"
#include "src/fibers-utils.h"

typedef struct {
    CPUState *cpu;
    void*(*func)(void*);
    void *arg;
} fiber_trampoline_args;

#define qemu_fiber pth_t
void *fiber_trampoline_set_cpu(void* arg);
void fiber_exit(bool continue_execution);

#ifdef AS_LIB
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