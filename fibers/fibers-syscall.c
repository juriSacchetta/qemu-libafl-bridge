#include <sys/prctl.h>

#include "qemu/osdep.h"
#include "qemu.h"
#include "user-internals.h"

#include <pth.h>
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

ssize_t fibers_syscall_pread64(int fd, void *buf, size_t count, off_t offset){
    //Use pth_pread to emulate pread64 seems to be safe
    FIBERS_LOG_DEBUG("pread fd: %d buf: %p count: %d offset: %d\n", fd, buf, count, offset);
    return pth_pread(fd, buf, count, offset);
}

ssize_t fibers_syscall_pwrite64(int fd, const void *buf, size_t count, off_t offset) {
    //Use pth_pwrite to emulate pwrite64 seems to be safe
    FIBERS_LOG_DEBUG("pwrite fd: %d buf: %p count: %d offset: %d\n", fd, buf, count, offset);
    return pth_pwrite(fd, buf, count, offset);
}

int fibers_syscall_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    FIBERS_LOG_DEBUG("connect sockfd: %d addr: %p addrlen: %d \n", sockfd, addr, addrlen);
    return pth_connect(sockfd, addr, addrlen);
}

int fibers_syscall_waitpid(pid_t pid, int *status, int options) {
    return pth_waitpid(pid, status, options);
}

ssize_t fibers_syscall_read(int fd, void *buf, size_t nbytes) {
    return pth_read(fd, buf, nbytes);
}

ssize_t fibers_syscall_readv(int fd, const struct iovec *iov, int iovcnt) {
    FIBERS_LOG_DEBUG("readv fd: %d iov: %p iovcnt: %d\n", fd, iov, iovcnt);
    return pth_readv(fd, iov, iovcnt);
}

ssize_t fibers_syscall_write(int fd, const void *buf, size_t nbytes) {
    return pth_write(fd, buf, nbytes);
}

ssize_t fibers_syscall_writev(int fd, const struct iovec *iov, int iovcnt) {
    FIBERS_LOG_DEBUG("writev fd: %d iov: %p iovcnt: %d\n", fd, iov, iovcnt);
    return pth_writev(fd, iov, iovcnt);
}

abi_long fibers_syscall_prctl(abi_long option, abi_long arg2, abi_long arg3, abi_long arg4, abi_long arg5) {
    pth_attr_t  attr;
    switch (option) {
        case PR_SET_NAME:
            attr = pth_attr_of(pth_self());
            pth_attr_set(attr, PTH_ATTR_NAME, (char *)arg2);
            return 0;
        case PR_GET_NAME:
            attr = pth_attr_of(pth_self());
            pth_attr_get(attr, PTH_ATTR_NAME, (char *)arg2);
            FIBERS_LOG_DEBUG("PR_GET_NAME\n");
            return 0;
        default:
            qemu_log("prctl: unknown option %ld\n", option);
            abort();
    }
}

int fibers_syscall_ppoll(struct pollfd *fds, nfds_t nfds, const struct timespec *timeout_ts, const sigset_t *sigmask) {
    //TODO: check if is same use pth_poll to emulate ppoll
    FIBERS_LOG_DEBUG("ppoll pfd: %p nfds: %d timeout: %ld\n", pfd, nfds, timeout_ts->tv_nsec/1000);
    return pth_poll(fds, nfds, timeout_ts->tv_nsec/1000);
}

int fibers_syscall_accept4(int sockfd, struct sockaddr *addr, socklen_t *addrlen, int flags) {
    //TODO: check is is safe use pth_accept to emulate accept4
    FIBERS_LOG_DEBUG("accept4 sockfd: %d addr: %p addrlen: %d flags: %d\n", sockfd, addr, addrlen, flags);
    return pth_accept(sockfd, addr, addrlen);
}
ssize_t fibers_syscall_sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen) {
    FIBERS_LOG_DEBUG("sendto sockfd: %d buf: %p len: %d flags: %d dest_addr: %p addrlen: %d\n", sockfd, buf, len, flags, dest_addr, addrlen);
    return pth_sendto(sockfd, buf, len, flags, dest_addr, addrlen);
}

ssize_t fibers_syscall_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {
    FIBERS_LOG_DEBUG("recvfrom sockfd: %d buf: %p len: %d flags: %d src_addr: %p addrlen: %d\n", sockfd, buf, len, flags, src_addr, addrlen);
    return pth_recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
}

int fibers_syscall_wait4(pid_t pid, int *status, int options, struct rusage *rusage) {
    //TODO: check if is same use pth_waitpid to emulate wait4
    FIBERS_LOG_DEBUG("wait4 pid: %d status: %p options: %d rusage: %p\n", pid, status, options, rusage);
    return pth_waitpid(pid, status, options);
}