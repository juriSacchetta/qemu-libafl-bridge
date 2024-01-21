#pragma once

#include "pth/pth.h"
#include "qemu/osdep.h"
#include "qemu.h"
#include "src/fibers-types.h"
#include "src/fibers-utils.h"

#ifndef QEMU_FIBERS
#define QEMU_FIBERS
#endif

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

void fibers_init(CPUArchState *env);
void fibers_fork_end(bool child);

int  fibers_register_thread(pth_t thread, CPUArchState *cpu);
bool fibers_unregister_thread(pth_t thread);

void fibers_call_scheduler(void);

int fibers_syscall_futex(int *uaddr, int op, int val, const struct timespec *timeout_ev, uint32_t val2, int *uaddr2, uint32_t val3);
int fibers_syscall_tkill(abi_long tid, abi_long sig);
int fibers_syscall_tgkill(abi_long arg1, abi_long arg2, abi_long arg3);
int fibers_syscall_gettid(void);
int fibers_syscall_nanosleep(struct timespec *ts);
int fibers_syscall_clock_nanosleep(clockid_t clock_id, struct timespec *ts);

abi_long fibers_syscall_prctl(abi_long option, abi_long arg2, abi_long arg3, abi_long arg4, abi_long arg5);

GEN_SYSCALL_WRAPPER_SIGN(ssize_t, pread64, int fd, void *buf, size_t nbytes, off_t offset)
GEN_SYSCALL_WRAPPER_SIGN(ssize_t, pwrite64, int fd, const void *buf, size_t nbytes, off_t offset)
GEN_SYSCALL_WRAPPER_SIGN(int, connect, int sockfd, const struct sockaddr *addr, socklen_t addrlen)
GEN_SYSCALL_WRAPPER_SIGN(int, waitpid, pid_t pid, int *status, int options)
GEN_SYSCALL_WRAPPER_SIGN(ssize_t, read, int fd, void *buf, size_t nbytes)
GEN_SYSCALL_WRAPPER_SIGN(ssize_t, readv, int fd, const struct iovec *iov, int iovcnt)
GEN_SYSCALL_WRAPPER_SIGN(ssize_t, write, int fd, const void *buf, size_t nbytes)
GEN_SYSCALL_WRAPPER_SIGN(ssize_t, writev, int fd, const struct iovec *iov, int iovcnt)
GEN_SYSCALL_WRAPPER_SIGN(int, ppoll, struct pollfd *fds, nfds_t nfds, const struct timespec *timeout_ts, const sigset_t *sigmask)
GEN_SYSCALL_WRAPPER_SIGN(int, accept4, int fd, struct sockaddr *addr, socklen_t *len, int flags)
GEN_SYSCALL_WRAPPER_SIGN(ssize_t, sendto, int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen)
GEN_SYSCALL_WRAPPER_SIGN(ssize_t, recvfrom, int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen)
GEN_SYSCALL_WRAPPER_SIGN(pid_t, wait4, pid_t pid, int *status, int options, struct rusage *rusage)