
#include "qemu/osdep.h"
#include "qemu.h"
#include "user-internals.h"

#include "pth/pth.h"
#include "fibers.h"
#include "src/fibers-types.h"
#include "src/fibers-thread.h"
#include "src/fibers-utils.h"

extern void force_sig_env(CPUArchState *env, int sig);
//TODO: check parametes 
int fibers_syscall_tkill(abi_long tid, abi_long sig) {
    assert(tid <= fibers_count);
    qemu_fiber *current = fibers_thread_by_tid(tid);
    if (current == NULL) return -ESRCH;
    force_sig_env(current->env, sig);
    return 0;
}

int fibers_syscall_tgkill(abi_long arg1, abi_long arg2, abi_long arg3) {
    assert(arg2 > BASE_FIBERS_TID);
    qemu_fiber *current = fibers_thread_by_tid(arg2);
    if(current == NULL) return -ESRCH;
    force_sig_env(current->env, arg3);
    return 0;
}

int fibers_syscall_gettid(void) {
    pth_t me = pth_self();
    qemu_fiber *current = fibers_thread_by_pth(me);
    assert(current != NULL);
    return current->fibers_tid;
}

int fibers_syscall_nanosleep(struct timespec *ts){
    FIBERS_LOG_DEBUG("nanosleep %ld %ld\n", ts->tv_sec, ts->tv_nsec/1000);
    return pth_nanosleep(ts, NULL); //TODO: Check this NULL
}

int fibers_syscall_clock_nanosleep(clockid_t clock_id, struct timespec *ts){
    FIBERS_LOG_DEBUG("clock_nanosleep %ld %ld\n", ts->tv_sec, ts->tv_nsec/1000);
    return pth_nanosleep(ts, NULL); //TODO: Check this NULL
}
int fibers_syscall_pread(int fd, void *buf, size_t nbytes, off_t offset) {
    return pth_pread(fd, buf, nbytes, offset);
}
int fibers_syscall_pwrite(int fd, const void *buf, size_t nbytes, off_t offset) {
    return pth_pwrite(fd, buf, nbytes, offset);
}

int fibers_syscall_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    return pth_connect(sockfd, addr, addrlen);
}

int fibers_syscall_waitpid(pid_t pid, int *status, int options) {
    return pth_waitpid(pid, status, options);
}

ssize_t fibers_syscall_read(int fd, void *buf, size_t nbytes) {
    return pth_read(fd, buf, nbytes);
}

ssize_t fibers_syscall_write(int fd, const void *buf, size_t nbytes) {
    return pth_write(fd, buf, nbytes);
}