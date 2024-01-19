#include "qemu/osdep.h"
#include "qemu/queue.h"
#include "qemu/log.h"
#include "qemu.h"

#include "pth/pth.h"
#include "fibers.h"
#include "src/fibers-futex.h"
#include "src/fibers-utils.h"

#define FUTEX_BITSET_MATCH_ANY 0xffffffff

// TODO: Check types, are you sure on the size?
// TODO: Check the timeout relative/absolute time
typedef struct fibers_futex
{
    pth_t pth_thread;
    int *uaddr;
    pth_cond_t cond;
    uint32_t bitset;
    QLIST_ENTRY(fibers_futex)
    entry;
} fibers_futex;

QLIST_HEAD(fiber_futex_list, fibers_futex)
futex_list;

pth_mutex_t futex_mutex;

void fibers_futex_init(void)
{
    QLIST_INIT(&futex_list);
    pth_mutex_init(&futex_mutex);
}

void fibers_clean_futex(void)
{
    pth_mutex_acquire(&futex_mutex, FALSE, NULL);
    fibers_futex *futex = NULL;
    QLIST_FOREACH(futex, &futex_list, entry)
    {
        free(futex);
    }
    QLIST_INIT(&futex_list);
    pth_mutex_release(&futex_mutex);
}

static inline bool match_futex(fibers_futex *futex, int *uaddr, uint32_t bitset)
{
    return futex && futex->uaddr == uaddr && (bitset == FUTEX_BITSET_MATCH_ANY || futex->bitset == bitset);
}

static int fibers_futex_wait(int *uaddr, int val, struct timespec *pts, uint32_t bitset)
{
    if (__atomic_load_n(uaddr, __ATOMIC_ACQUIRE) != val)
        return -TARGET_EAGAIN;

    pth_mutex_acquire(&futex_mutex, FALSE, NULL);

    fibers_futex *futex = malloc(sizeof(fibers_futex));
    memset(futex, 0, sizeof(fibers_futex));
    futex->pth_thread = pth_self();
    futex->uaddr = uaddr;
    futex->bitset = bitset;

    pth_cond_init(&futex->cond);

    QLIST_INSERT_HEAD(&futex_list, futex, entry);

    pth_event_t timeout = NULL;
    if (pts != NULL)
    {
        timeout = pth_event(PTH_EVENT_TIME, pth_timeout(pts->tv_sec, pts->tv_nsec / 1000));
    }
    pth_cond_await(&futex->cond, &futex_mutex, timeout);

    QLIST_REMOVE(futex, entry);
    free(futex);

    pth_mutex_release(&futex_mutex);
    return 0;
}

static int fibers_futex_wake(int *uaddr, int val, uint32_t bitset)
{
    if (!bitset)
        return -EINVAL;

    pth_mutex_acquire(&futex_mutex, FALSE, NULL);
    int count = 0;
    fibers_futex *current = NULL;
    QLIST_FOREACH(current, &futex_list, entry)
    {
        if (!match_futex(current, uaddr, bitset))
            continue;
        count++;
        pth_cond_notify(&current->cond, TRUE);
    }
    pth_mutex_release(&futex_mutex);
    return count;
}

static int fibers_futex_requeue(int op, int *uaddr, uint32_t val, int *uaddr2, int val2, int val3)
{
    fibers_futex *current = NULL;
    if (op == FUTEX_CMP_REQUEUE && __atomic_load_n(uaddr, __ATOMIC_ACQUIRE) != val3)
        return -TARGET_EAGAIN;
    int count_ops = 0;
    int count_requeqe = 0;
    QLIST_FOREACH(current, &futex_list, entry)
    {
        if (!match_futex(current, uaddr, FUTEX_BITSET_MATCH_ANY))
            continue;
        if (count_ops < val)
        {
            count_ops++;
            pth_cond_notify(&current->cond, TRUE);
        }
        else if (val2 != -1 && count_requeqe < val2)
        {
            current->uaddr = uaddr2;
            count_requeqe++;
            if (op == FUTEX_CMP_REQUEUE)
                count_ops++;
        }
    }
    return count_ops;
}

static inline bool valid_timeout(const struct timespec *timeout) {
    if(timeout == NULL) return true;
    if(timeout->tv_sec < 0 || timeout->tv_nsec < 0 || timeout->tv_nsec >= 1000000000) return false;
    return true;
}

int fibers_syscall_futex(int *uaddr, int op, int val, const struct timespec *timeout, uint32_t val2, int *uaddr2, uint32_t val3)
{
    if(!valid_timeout(timeout)){
        return -TARGET_EINVAL;
    }
    switch (op)
    {
    case FUTEX_WAIT:
        val3 = FUTEX_BITSET_MATCH_ANY;
    case FUTEX_WAIT_BITSET:
        FIBERS_LOG_DEBUG("futex wait_bitset uaddr: %p val: %d val3: %p\n", uaddr, val, val3);
        return fibers_futex_wait(uaddr, val, (struct timespec *)timeout, val3);
    case FUTEX_WAKE:
        val3 = FUTEX_BITSET_MATCH_ANY;
    case FUTEX_WAKE_BITSET:
        return fibers_futex_wake(uaddr, val, val3);
    case FUTEX_REQUEUE:
        return fibers_futex_requeue(op, uaddr, val, uaddr2, val2, val3);
    case FUTEX_CMP_REQUEUE:
        FIBERS_LOG_DEBUG("futex FUTEX_CMP_REQUEUE uaddr: %p val: %d uaddr2: %p val3: %p\n", uaddr, val, uaddr2, val3);
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