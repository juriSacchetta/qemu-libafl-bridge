#pragma once

#include "pth/pth.h"
#include "qemu/osdep.h"
#include "qemu.h"
#include "src/fibers-types.h"
#include "src/fibers-utils.h"

#ifndef QEMU_FIBERS
#define QEMU_FIBERS 
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

void fibers_init(CPUArchState *env);
int  fibers_new_thread(pth_t thread, CPUArchState *cpu);
void fibers_thread_clear_all(void);

void fibers_call_scheduler(void);

int fibers_syscall_futex(int *uaddr, int op, int val, const struct timespec *timeout, target_ulong val2, int *uaddr2, int val3);
int fibers_syscall_tkill(abi_long tid, abi_long sig);
int fibers_syscall_tgkill(abi_long arg1, abi_long arg2, abi_long arg3);
int fibers_syscall_gettid(void);
int fibers_syscall_nanosleep(struct timespec *ts);
int fibers_syscall_clock_nanosleep(clockid_t clock_id, struct timespec *ts);

GEN_SYSCALL_WRAPPER_SIGN(int, pread, int fd, void *buf, size_t nbytes, off_t offset)
GEN_SYSCALL_WRAPPER_SIGN(int, pwrite, int fd, const void *buf, size_t nbytes, off_t offset)
GEN_SYSCALL_WRAPPER_SIGN(int, connect, int sockfd, const struct sockaddr *addr, socklen_t addrlen)
GEN_SYSCALL_WRAPPER_SIGN(int, waitpid, pid_t pid, int *status, int options)
GEN_SYSCALL_WRAPPER_SIGN(ssize_t, read, int fd, void *buf, size_t nbytes)
GEN_SYSCALL_WRAPPER_SIGN(ssize_t, write, int fd, const void *buf, size_t nbytes)