#include "qemu/osdep.h"
#include "qemu.h"
#include "user-internals.h"
#include "qemu/guest-random.h"

#include "cpu_loop-common.h"
#include "qemu/queue.h"

#include "pth.h"
#include "fibers.h"

void force_sig_env(CPUArchState *env, int sig);

typedef struct qemu_fiber{
    CPUArchState *env;
    int fibers_tid;
    pth_t thread;
    QLIST_ENTRY(qemu_fiber) entry;
} qemu_fiber;
QLIST_HEAD(qemu_fiber_list, qemu_fiber) fiber_list_head;

typedef struct fibers_futex {
    qemu_fiber *fiber;
    int *futex_uaddr;
    uint32_t mask;
    bool waiting;
    QLIST_ENTRY(fibers_futex) entry;
} fibers_futex;
QLIST_HEAD(fiber_futex_list, fibers_futex) futex_list;

typedef struct qemu_fiber_fd {
    int fd;
    int original_flags;
    QLIST_ENTRY(qemu_fiber_fd) entry;
} qemu_fiber_fd;

#define BASE_FIBERS_TID 0x3fffffff
qemu_fiber *fibers = NULL;

int fibers_count = BASE_FIBERS_TID;

static qemu_fiber * search_fiber_by_fibers_tid(int fibers_tid) {
    qemu_fiber *current;
    QLIST_FOREACH(current, &fiber_list_head, entry) {
        if (current->fibers_tid == fibers_tid) return current;
    }
    return NULL;
}

static qemu_fiber * search_fiber_by_pth(pth_t thread) {
    qemu_fiber *current;
    QLIST_FOREACH(current, &fiber_list_head, entry) {
        if (current->thread == thread) return current;
    }
    return NULL;
}

void qemu_fibers_init(CPUArchState *env)
{
    QLIST_INIT(&fiber_list_head);
    QLIST_INIT(&futex_list);
    qemu_fiber * new = malloc(sizeof(qemu_fiber));
    memset(new, 0, sizeof(qemu_fiber));

    pth_save_thread_cpu_addr((uintptr_t *)&thread_cpu);

    new->env = env;
    new->fibers_tid = fibers_count;
    new->thread = pth_init((uintptr_t)env_cpu(env));
    QLIST_INSERT_HEAD(&fiber_list_head, new, entry);
}

/*
Thread Control
*/

int register_fiber(pth_t thread, CPUArchState *cpu) {
    qemu_fiber *new = malloc(sizeof(qemu_fiber));
    memset(new, 0, sizeof(qemu_fiber));
    new->env = cpu;
    new->thread = thread;
    new->fibers_tid = ++fibers_count;
    QLIST_INSERT_HEAD(&fiber_list_head, new, entry);
    return new->fibers_tid;
}
static fibers_futex * create_futex(int *uaddr, uint32_t mask) {
    fibers_futex *new = malloc(sizeof(fibers_futex));
    memset(new, 0, sizeof(fibers_futex));
    new->fiber = search_fiber_by_pth(pth_self());
    new->futex_uaddr = uaddr;
    new->waiting = true;
    new->mask = mask;
    QLIST_INSERT_HEAD(&futex_list, new, entry);
    return new;
}

static void destroy_futex(fibers_futex *futex) {
    QLIST_REMOVE(futex, entry);
    free(futex);
}

//TODO: merge this function with fibers_futex_wait_bitset
static int fibers_futex_wait(int* uaddr, int val, struct timespec *pts, uint32_t mask) {
    fibers_futex *futex = NULL;

    while(__atomic_load_n(uaddr, __ATOMIC_ACQUIRE) == val) {
        if(!futex) {
            futex = create_futex(uaddr, mask);
        } else if(futex->waiting == false) {
            destroy_futex(futex);
            futex = NULL;
            return 0;
        }
        if (pts != NULL) {
            pth_wait(pth_event(PTH_EVENT_TIME, pth_timeout(pts->tv_sec, pts->tv_nsec/1000)));
        } else {
            pth_yield(NULL);
        }
    }
    return -TARGET_EAGAIN;
}

static int fibers_futex_wake(int* uaddr, int val, uint32_t mask) {
    int count = 0;
    fibers_futex *current;
    QLIST_FOREACH(current, &futex_list, entry) {
        if((mask != 0) && (current->mask & mask) == 0) continue;
        current->waiting = false;
        if(count == val) break;
        else count++;
    }
    pth_yield(NULL);
    return count;
}


// static int fibers_futex_requeue(CPUState *cpu, int base_op, int val, int val3,
//                     target_ulong uaddr, 
//                     target_ulong uaddr2, target_ulong timeout) {
    // uint32_t val2 = (uint32_t)timeout;
    // int* to_check = (int*)g2h(cpu, uaddr);
    // int expected = tswap32(val3);
    // if (base_op == FUTEX_CMP_REQUEUE &&
    //     __atomic_load_n(to_check, __ATOMIC_ACQUIRE) != expected)
    //     return -EAGAIN;
    // int i, j, k;
    // for (i = 0, j = 0, k = 0; i < fibers_count; ++i) {
    //     if (fibers[i].is_zombie) continue;
    //     if (fibers[i].waiting_futex && fibers[i].futex_addr == uaddr) {
    //         if (j < val) {
    //             ++j;
    //             fibers[i].waiting_futex = 0;
    //         } else if (k < val2) {
    //             ++k;
    //             fibers[i].futex_addr = uaddr2;
    //         }
    //     }
    // }
    // if (val) qemu_fibers_switch();
    //return j+k;
//     return 0;
// }

int fibers_do_futex(int *uaddr, int op, int val, const struct timespec *timeout, int *uaddr2, int val3) {
    switch (op) {
        case FUTEX_WAIT:
            fprintf(stderr, "qemu_fiber: futex wait 0x%p %d\n", uaddr, val);
            return fibers_futex_wait(uaddr, val, (struct timespec *)timeout, 0);
        case FUTEX_WAIT_BITSET:
            fprintf(stderr, "qemu_fiber: futex wait_bitset %p %d %d\n", uaddr, val, val3);
            return fibers_futex_wait(uaddr, val, (struct timespec *)timeout, val3);
        case FUTEX_WAIT_REQUEUE_PI:
        case FUTEX_LOCK_PI:
        case FUTEX_LOCK_PI2:
        case FUTEX_WAKE:
            fprintf(stderr, "qemu_fiber: futex wake %p %d\n", uaddr, val);
            return fibers_futex_wake(uaddr, val, 0);
        case FUTEX_WAKE_BITSET:
            fprintf(stderr, "qemu_fiber: futex wake_bitset %p %d %d\n", uaddr, val, val3);
            return fibers_futex_wake(uaddr, val, val3);
        case FUTEX_TRYLOCK_PI:
        case FUTEX_UNLOCK_PI:
        case FUTEX_FD:
        case FUTEX_CMP_REQUEUE:
        case FUTEX_CMP_REQUEUE_PI:
        case FUTEX_REQUEUE:
        case FUTEX_WAKE_OP:
            break;
        default:
            return -TARGET_ENOSYS;
    }
    return -TARGET_ENOSYS;
}

/*
## Syscall replacements
*/

void fibers_syscall_exit(void *value) {
    pth_exit(value);
}

//TODO: check parametes 
int fibers_syscall_tkill(abi_long tid, abi_long sig) {
    if (tid > fibers_count) exit(-1); //TODO: Improve this error managing
    qemu_fiber *current = search_fiber_by_fibers_tid(tid);
    if (current == NULL) return -ESRCH;
    force_sig_env(current->env, sig);
    return 0;
}
//TODO: check parametes 
int fibers_syscall_tgkill(abi_long arg1, abi_long arg2, abi_long arg3) {
    if (arg2 > fibers_count) exit(-1); //TODO: Improve this error managing
    qemu_fiber *current = search_fiber_by_fibers_tid(arg2);
    if(current == NULL) return -ESRCH;
    force_sig_env(current->env, arg3);
    return 0;
}

int fibers_syscall_gettid(void) {
    pth_t me = pth_self();
    qemu_fiber *current = search_fiber_by_pth(me);
    if (current == NULL) exit(-1); //TODO: Logic error
    return current->fibers_tid;
}

int fibers_syscall_nanosleep(struct timespec *ts){
    fprintf(stderr, "qemu_fiber: nanosleep %ld %ld\n", ts->tv_sec, ts->tv_nsec/1000);
    return pth_nanosleep(ts, NULL); //TODO: Check this NULL
}

int fibers_syscall_clock_nanosleep(clockid_t clock_id, struct timespec *ts){
    fprintf(stderr, "qemu_fiber: clock_nanosleep %ld %ld\n", ts->tv_sec, ts->tv_nsec/1000);
    return pth_nanosleep(ts, NULL); //TODO: Check this NULL
}

int fibers_syscall_pread(int fd, void *buf, size_t nbytes, off_t offset) {
    return pth_pread(fd, buf, nbytes, offset);
}
int fibers_syscall_pwrite(int fd, const void *buf, size_t nbytes, off_t offset) {
    return pth_pwrite(fd, buf, nbytes, offset);
}

int fibers_syscall_pread64(int fd, void *buf, size_t nbytes, off_t offset) {
    return fibers_syscall_pread(fd, buf, nbytes, offset);
}
int fibers_syscall_pwrite64(int fd, const void *buf, size_t nbytes, off_t offset) {
    return fibers_syscall_pwrite(fd, buf, nbytes, offset);
}

/*##########
# I/O OPERATIONS
###########*/
ssize_t fibers_read(int fd, void *buf, size_t nbytes) {
    return pth_read(fd, buf, nbytes);
}

ssize_t fibers_write(int fd, const void *buf, size_t nbytes) {
    return pth_write(fd, buf, nbytes);
}