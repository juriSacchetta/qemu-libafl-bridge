#include "qemu/osdep.h"
#include "qemu/queue.h"
#include "qemu.h"

#include "pth/pth.h"
#include "fibers.h"
#include "src/fibers-futex.h"
#include "src/fibers-utils.h"


typedef struct fibers_futex {
    pth_t pth_thread;
    int *futex_uaddr;
    pth_cond_t cond;
    uint32_t mask;
    QLIST_ENTRY(fibers_futex) entry;
} fibers_futex;
QLIST_HEAD(fiber_futex_list, fibers_futex) futex_list;

pth_mutex_t futex_mutex;


void fibers_futex_init(void) {
    QLIST_INIT(&futex_list);
    pth_mutex_init(&futex_mutex);
}

static fibers_futex * create_futex(int *uaddr, uint32_t mask) {
    fibers_futex *new = malloc(sizeof(fibers_futex));
    memset(new, 0, sizeof(fibers_futex));
    new->pth_thread = pth_self();
    new->futex_uaddr = uaddr;
    pth_cond_init(&new->cond);
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

    if(__atomic_load_n(uaddr, __ATOMIC_ACQUIRE) == val) {
        pth_mutex_acquire(&futex_mutex, FALSE, NULL);

        futex = create_futex(uaddr, mask);
        //TODO: controllare come viene usato questo timeout
        pth_event_t timeout = NULL;
        if (pts != NULL) {
            timeout = pth_event(PTH_EVENT_TIME, pth_timeout(pts->tv_sec, pts->tv_nsec/1000));
        }
        pth_cond_await(&futex->cond, &futex_mutex, timeout);
        destroy_futex(futex);

        pth_mutex_release(&futex_mutex);
        return 0;
    }
    return -TARGET_EAGAIN;
}

static int fibers_futex_wake(int* uaddr, int val, uint32_t mask) {
    pth_mutex_acquire(&futex_mutex, FALSE, NULL);
    int count = 0;
    fibers_futex *current = NULL;
    QLIST_FOREACH(current, &futex_list, entry) {
        if(count == val) break;
        if((uaddr != current->futex_uaddr) || ((mask != 0) && (current->mask & mask) == 0)) continue;        
        count++;
        pth_cond_notify(&current->cond, TRUE);
    }
    pth_mutex_release(&futex_mutex);
    return count;
}

static int fibers_futex_requeue(int op, int *uaddr, uint32_t val, int *uaddr2, int val2, int val3) {
    fibers_futex *current = NULL;
    if (op == FUTEX_CMP_REQUEUE &&
        __atomic_load_n(uaddr, __ATOMIC_ACQUIRE) != val3)
        return -TARGET_EAGAIN;
    int count_ops = 0;
    int count_requeqe = 0;
    QLIST_FOREACH(current, &futex_list, entry) {
        if(uaddr != current->futex_uaddr) continue; //|| ((mask != 0) && (current->mask & mask) == 0)
        if(count_ops < val) {
            count_ops++;
            pth_cond_notify(&current->cond, TRUE);
        } else if(count_requeqe < val2 ){
            current->futex_uaddr = uaddr2;
            count_requeqe++;
            count_ops++;
        }
    }
    return count_ops;
}

int fibers_syscall_futex(int *uaddr, int op, int val, const struct timespec *timeout, target_ulong val2, int *uaddr2, int val3) {
    switch (op) {
        case FUTEX_WAIT:
            FIBERS_LOG_DEBUG("futex wait uaddr: %p val: %d\n", uaddr, val);
            return fibers_futex_wait(uaddr, val, (struct timespec *)timeout, 0);
        case FUTEX_WAIT_BITSET:
            FIBERS_LOG_INFO("futex wait_bitset uaddr: %p val: %d val3: %d\n", uaddr, val, val3);
            return fibers_futex_wait(uaddr, val, (struct timespec *)timeout, val3);
        case FUTEX_WAKE:
            FIBERS_LOG_DEBUG("futex wake uaddr: %p val: %d\n", uaddr, val);
            return fibers_futex_wake(uaddr, val, 0);
        case FUTEX_WAKE_BITSET:
            FIBERS_LOG_INFO("futex wake_bitset uaddr: %p val: %d val3: %d\n", uaddr, val, val3);
            return fibers_futex_wake(uaddr, val, val3);
        case FUTEX_REQUEUE:
        case FUTEX_CMP_REQUEUE:
            FIBERS_LOG_DEBUG("futex FUTEX_CMP_REQUEUE uaddr: %p val: %d uaddr2: %p val3: %d\n", uaddr, val, uaddr2, val3);
            return fibers_futex_requeue(op, uaddr, val, uaddr2, val2, val3);
        case FUTEX_WAIT_REQUEUE_PI:
        case FUTEX_LOCK_PI:
        case FUTEX_LOCK_PI2:
        case FUTEX_TRYLOCK_PI:
        case FUTEX_UNLOCK_PI:
        case FUTEX_FD:
        case FUTEX_CMP_REQUEUE_PI:
        case FUTEX_WAKE_OP:
            exit(-1);
            break;
        default:
            return -TARGET_ENOSYS;
    }
    return -TARGET_ENOSYS;
}
