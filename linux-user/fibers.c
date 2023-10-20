#include "qemu/osdep.h"
#include "qemu.h"
#include "user-internals.h"
#include "qemu/guest-random.h"

#include "cpu_loop-common.h"
#include "qemu/queue.h"

//#include "fibers.h"
#include "pth.h"

typedef struct {
    CPUArchState *env;
    int tid;
    abi_ulong child_tidptr;
    abi_ulong parent_tidptr;
    sigset_t sigmask;
} new_thread_info;

typedef struct qemu_fiber{
    CPUArchState *env;
    int fibers_tid;
    pth_t thread;
    QSLIST_ENTRY(qemu_fiber) next;
} qemu_fiber;

typedef struct qemu_fiber_fd {
    int fd;
    int original_flags;
    QLIST_ENTRY(qemu_fiber_fd) next;
} qemu_fiber_fd;

#include "fibers.h"

#define BASE_FIBERS_TID 0x3fffffff
qemu_fiber *fibers = NULL;
QSLIST_HEAD(qemu_fiber_list, qemu_fiber) fiber_list_head;
int fibers_count = BASE_FIBERS_TID;



/*
## Utility func
*/

static qemu_fiber * search_fiber_by_fibers_tid(int fibers_tid) {
    qemu_fiber *current;
    QSLIST_FOREACH(current, &fiber_list_head, next) {
        if (current->fibers_tid == fibers_tid) return current;
    }
    return NULL;
}

static qemu_fiber * search_fiber_by_pth(pth_t thread) {
    qemu_fiber *current;
    QSLIST_FOREACH(current, &fiber_list_head, next) {
        if (current->thread == thread) return current;
    }
    return NULL;
}


void force_sig_env(CPUArchState *env, int sig);


/*##########
## FIBERS ##
###########*/

void qemu_fibers_init(CPUArchState *env)
{
    QSLIST_INIT(&fiber_list_head);
    qemu_fiber * new = malloc(sizeof(qemu_fiber));
    memset(new, 0, sizeof(qemu_fiber));

    new->env = env;
    new->fibers_tid = 0;
    new->thread = pth_init();
    QSLIST_INSERT_HEAD(&fiber_list_head, new, next);
}

static void qemu_fibers_exec(void *fiber_instance)
{
    CPUState *cpu;
    TaskState *ts;
    
    qemu_fiber *current = ((qemu_fiber *) fiber_instance);
    cpu = env_cpu(current->env);
    ts = (TaskState *)cpu->opaque;
    task_settid(ts);
    thread_cpu = cpu;

    fprintf(stderr, "qemu_fibers: starting 0x%d\n", current->fibers_tid - BASE_FIBERS_TID);
    cpu_loop(current->env);
}


int qemu_fibers_spawn(void *info_void)
{
    //TODO: Creare una interfacia unica per new_thread_info
    new_thread_info *info = (new_thread_info*)info_void;
    CPUArchState *env;
    CPUState *cpu;

    int new_tid = fibers_count++;
    env = info->env;
    cpu = env_cpu(env);
    info->tid = new_tid; // sys_gettid();
    if (info->child_tidptr)
        put_user_u32(info->tid, info->child_tidptr);
    if (info->parent_tidptr)
        put_user_u32(info->tid, info->parent_tidptr);
    qemu_guest_random_seed_thread_part2(cpu->random_seed);

    qemu_fiber *new = malloc(sizeof(qemu_fiber));
    memset(new, 0, sizeof(qemu_fiber));

    QSLIST_INSERT_HEAD(&fiber_list_head, new, next);
    
    new->env = env;
    new->fibers_tid = new_tid;
    new->thread = pth_spawn(PTH_ATTR_DEFAULT, (void* (*) (void*))qemu_fibers_exec, (void *) new);
    return new_tid;
}

/*
Thread Control
*/

int fibers_futex_wait(CPUState *cpu, target_ulong uaddr, int op, int val, struct timespec *pts) {

        int* to_check = (int*)g2h(cpu, uaddr);
        int expected = tswap32(val);

        fprintf(stderr, "qemu_fiber: futex wait 0x%lx %d\n", uaddr, expected);

        while(__atomic_load_n(to_check, __ATOMIC_ACQUIRE) == expected) {
            if (pts != NULL) {
                pth_wait(pth_event(PTH_EVENT_TIME, pts->tv_sec, pts->tv_nsec/1000));
                return 0;
            }
            pth_yield(NULL);
            to_check = (int*)g2h(cpu, uaddr);
        }
        return 0;
}

int fibers_futex_requeue(CPUState *cpu, int base_op, int val, int val3,
                    target_ulong uaddr, 
                    target_ulong uaddr2, target_ulong timeout) {
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
    return 0;
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


/*##########
# I/O OPERATIONS
###########*/
ssize_t fibers_read(int fd, void *buf, size_t nbytes) {
    return pth_read(fd, buf, nbytes);
}

ssize_t fibers_write(int fd, const void *buf, size_t nbytes) {
    return pth_write(fd, buf, nbytes);
}