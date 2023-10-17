#include "qemu/osdep.h"
#include "qemu.h"
#include "user-internals.h"
#include "qemu/guest-random.h"

#include "cpu_loop-common.h"

typedef struct {
    CPUArchState *env;
    uint32_t tid;
    abi_ulong child_tidptr;
    abi_ulong parent_tidptr;
    sigset_t sigmask;
} new_thread_info;


struct qemu_fiber {
    CPUArchState *env;
    ucontext_t ctx;
    int is_main;
    int is_zombie; // TODO reuse zombie slots on create
    int waiting_futex;
    target_ulong futex_addr;
};

#include "fibers.h"

#ifndef NEW_STACK_SIZE
#define NEW_STACK_SIZE 0x40000
#endif

#undef timespeccmp
#undef timespecsub
#define	timespeccmp(tsp, usp, cmp)					\
	(((tsp)->tv_sec == (usp)->tv_sec) ?				\
	    ((tsp)->tv_nsec cmp (usp)->tv_nsec) :			\
	    ((tsp)->tv_sec cmp (usp)->tv_sec))
#define	timespecsub(tsp, usp, vsp)					\
	do {								\
		(vsp)->tv_sec = (tsp)->tv_sec - (usp)->tv_sec;		\
		(vsp)->tv_nsec = (tsp)->tv_nsec - (usp)->tv_nsec;	\
		if ((vsp)->tv_nsec < 0) {				\
			(vsp)->tv_sec--;				\
			(vsp)->tv_nsec += 1000000000L;			\
		}							\
	} while (0)

int fiber_current = 0;
int fibers_count = 0;
int fibers_active_count = 0;
struct qemu_fiber *fibers = NULL;
int fiber_last_switch = 0;

#define FIBER_CLOCK_SWICTH (2500000000)

void force_sig_env(CPUArchState *env, int sig);

void qemu_fibers_init(CPUArchState *env)
{
    int idx = fibers_count++;
    fibers_active_count++;
    fibers = realloc(fibers, fibers_count * sizeof(struct qemu_fiber));
    memset(&fibers[idx], 0, sizeof(struct qemu_fiber));
    fibers[idx].env = env;
    fibers[idx].is_main = 1;
    getcontext(&fibers[idx].ctx); // needed?
    fiber_current = idx;
    fiber_last_switch = clock();
}

static void qemu_fibers_kill(int idx)
{
    fibers[idx].is_zombie = 1;
    --fibers_active_count;
}

static void qemu_fibers_switch(void)
{
    if (fibers_active_count < 2) return;

    fiber_last_switch = clock();
    int old = fiber_current;
    do {
        fiber_current = (fiber_current +1) % fibers_count;
        if (fiber_current == old)
            return;
    } while (fibers[fiber_current].is_zombie);
    fprintf(stderr, "qemu_fibers: swap from %d to %d\n", old, fiber_current);
    thread_cpu = env_cpu(fibers[fiber_current].env);
    swapcontext(&fibers[old].ctx, &fibers[fiber_current].ctx);
}

void qemu_fibers_may_switch(void) {
    if (clock() - fiber_last_switch > FIBER_CLOCK_SWICTH)
        qemu_fibers_switch();
}

static void qemu_fibers_exec(int idx)
{
    CPUState *cpu;
    TaskState *ts;
    
    fiber_current = idx;
    cpu = env_cpu(fibers[idx].env);
    ts = (TaskState *)cpu->opaque;
    task_settid(ts);
    thread_cpu = cpu;

    fprintf(stderr, "qemu_fibers: starting %d\n", fiber_current);
    cpu_loop(fibers[idx].env);
}

int qemu_fibers_create(void *info_void)
{
    //Creare una interfacia unica per new_thread_info
    new_thread_info *info = (new_thread_info*)info_void;
    CPUArchState *env;
    CPUState *cpu;

    int idx = fibers_count++;
    ++fibers_active_count;
    int new_tid = 0x3fffffff + idx;

    env = info->env;
    cpu = env_cpu(env);
    info->tid = new_tid; // sys_gettid();
    if (info->child_tidptr)
        put_user_u32(info->tid, info->child_tidptr);
    if (info->parent_tidptr)
        put_user_u32(info->tid, info->parent_tidptr);
    qemu_guest_random_seed_thread_part2(cpu->random_seed);

    void *stack = malloc(NEW_STACK_SIZE);

    fibers = realloc(fibers, fibers_count * sizeof(struct qemu_fiber));
    memset(&fibers[idx], 0, sizeof(struct qemu_fiber));
    
    getcontext(&fibers[idx].ctx);
    
    fibers[idx].env = env;
    fibers[idx].ctx.uc_stack.ss_sp = stack;
    fibers[idx].ctx.uc_stack.ss_size = NEW_STACK_SIZE;
    fibers[idx].ctx.uc_link = NULL;
    makecontext(&fibers[idx].ctx, (void (*) (void))&qemu_fibers_exec, 1, idx);
    
    swapcontext(&fibers[fiber_current].ctx, &fibers[idx].ctx);
    
    return new_tid;
}


//patches for syscalls and futexes

int fibers_futex_wait(CPUState *cpu, target_ulong uaddr, int op, int val, struct timespec *pts) {
        struct timespec futex_start;
        clockid_t clock_type = CLOCK_MONOTONIC;
        if (op & FUTEX_CLOCK_REALTIME)
            clock_type = CLOCK_REALTIME;
        if (pts)
            clock_gettime(clock_type, &futex_start);
        int* to_check = (int*)g2h(cpu, uaddr);
        int expected = tswap32(val);
        fibers[fiber_current].futex_addr = uaddr;
        fprintf(stderr, "qemu_fiber: futex wait 0x%lx %d\n", uaddr, expected);
        while (__atomic_load_n(to_check, __ATOMIC_ACQUIRE) == expected) {
            if (pts) {
                struct timespec now, diff;
                clock_gettime(clock_type, &now);
                timespecsub(&now, &futex_start, &diff);
                if (timespeccmp(&diff, pts, >=)) {
                    fibers[fiber_current].waiting_futex = 0;
                    return 0;
                }
            }
            fibers[fiber_current].waiting_futex = 1;
            qemu_fibers_switch();
            if (!fibers[fiber_current].waiting_futex)
                break;
            to_check = (int*)g2h(cpu, fibers[fiber_current].futex_addr);
        }
        return 0;
}

int fibers_futex_requeue(CPUState *cpu, int base_op, int val, int val3,
                    target_ulong uaddr, 
                    target_ulong uaddr2, target_ulong timeout) {
    uint32_t val2 = (uint32_t)timeout;
    int* to_check = (int*)g2h(cpu, uaddr);
    int expected = tswap32(val3);
    if (base_op == FUTEX_CMP_REQUEUE &&
        __atomic_load_n(to_check, __ATOMIC_ACQUIRE) != expected)
        return -EAGAIN;
    int i, j, k;
    for (i = 0, j = 0, k = 0; i < fibers_count; ++i) {
        if (fibers[i].is_zombie) continue;
        if (fibers[i].waiting_futex && fibers[i].futex_addr == uaddr) {
            if (j < val) {
                ++j;
                fibers[i].waiting_futex = 0;
            } else if (k < val2) {
                ++k;
                fibers[i].futex_addr = uaddr2;
            }
        }
    }
    if (val) qemu_fibers_switch();
    return j+k;
}


void fibers_syscall_exit(void) {
       fprintf(stderr, "qemu_fibers: exit %d\n", fiber_current);
        qemu_fibers_kill(fiber_current);
        while (1) qemu_fibers_switch();
}

int fibers_syscall_tkill(abi_long arg1, abi_long arg2) {
    int idx = arg1 - 0x3fffffff;
    if (idx > fibers_count) exit(-1); 
    if (fibers[idx].is_zombie) return -ESRCH;
    force_sig_env(fibers[idx].env, arg2);
    return 0;
}

int fibers_syscall_tgkill(abi_long arg1, abi_long arg2, abi_long arg3) {
    int idx = arg2 - 0x3fffffff;
    if (idx > fibers_count) exit(-1);
    if (fibers[idx].is_zombie)
        return -ESRCH;
    force_sig_env(fibers[idx].env, arg3);
    return 0;
}

int fibers_syscall_gettid(void) {
    return (0x3fffffff + fiber_current);
}

int fibers_syscall_clock_nanosleep(clockid_t clock_id, struct timespec *ts){
        struct timespec sleep_start;
        clock_gettime(clock_id, &sleep_start);
        fprintf(stderr, "qemu_fiber: clock_nanosleep %ld %ld\n", ts->tv_sec, ts->tv_nsec);
        while (1) {
            struct timespec now, diff;
            clock_gettime(clock_id, &now);
            timespecsub(&now, &sleep_start, &diff);
            if (timespeccmp(&diff, ts, >=))
                break;
            qemu_fibers_switch();
        }
        return 0;
}