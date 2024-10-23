#include "fibers.h"

#include "pth/pth.h"
#include "qemu/osdep.h"
#include "src/fibers-futex.h"
#include "src/fibers-thread.h"
#include "src/fibers-utils.h"

void fibers_init(CPUState *cpu)
{
   fibers_futex_init();
   fibers_thread_init(cpu);
}

void fibers_call_scheduler(void)
{
    int available_threads =
        pth_ctrl(PTH_CTRL_GETTHREADS_NEW | PTH_CTRL_GETTHREADS_READY | PTH_CTRL_GETTHREADS_SUSPENDED);
    if (available_threads > 0)
    {
        // FIBERS_LOG_DEBUG("Calling scheduler num: %d\n", random_number);
        pth_yield(NULL);
    }
}

void fibers_fork_end(bool child)
{
    if (child)
    {
        fibers_thread_clear_all();
        fibers_clean_futex();
    }
}

#ifdef AS_LIB
#include "libafl/cpu.h"
static bool initialized = false;
qemu_fiber *fiber_stopped = NULL;
extern CPUArchState *libafl_qemu_env;

extern void cpu_loop(CPUArchState *env);

void *fibers_cpu_loop(void *arg)
{
    CPUArchState *env = (CPUArchState *)arg;
    cpu_loop(env);
    fiber_exit(false);
    return NULL;
}

void fibers_save_stopped_thread(CPUArchState *cpu)
{
    fiber_stopped = fiber_by_pth(pth_self());
    fiber_stopped->stopped = true;
    fiber_stopped->env = cpu;
}
static int check_exit_condition(void *arg)
{
    return fiber_stopped != NULL;
}

int libafl_qemu_run(void)
{
    if ( likely(fiber_stopped != NULL))
    {
        pth_attr_t attr = pth_attr_new();
        pth_attr_set(attr, PTH_ATTR_JOINABLE, 0);
        fiber_stopped->thread = pth_spawn(attr, env_cpu(fiber_stopped->env), fibers_cpu_loop, fiber_stopped->env);
        pth_attr_destroy(attr);
        fiber_stopped->stopped = false;
        fiber_stopped = NULL;
    }
    else if(unlikely(!initialized))
    {
        fiber_spawn_cpu_loop(libafl_qemu_env);
        initialized = true;
    }
    else
    {
        assert(false);
    }
    pth_wait(pth_event(PTH_EVENT_FUNC, check_exit_condition, NULL, pth_time(0, 500000)));
    if(fiber_stopped != NULL)
    {
        pth_abort(fiber_stopped->thread);
    }

    // CPUState *some_cpu;
    // printf("!!!!!!!!!!CPU!!!!!!!!!!!!!\n");
    // CPU_FOREACH(some_cpu)
    // {
    //     printf("CPU: %p\n", some_cpu);
    
    // }
    // printf("!!!!!!!!!!CPU!!!!!!!!!!!!!\n");

    return 0;
}

int fibers_get_tid_by_cpu(CPUArchState *cpu)
{
    qemu_fiber *current;
    QLIST_FOREACH(current, &fiber_list_head, entry)
    {
        if (current->env == cpu)
        {
            return current->fiber_tid;
        }
    }
    return -1;
}
#endif