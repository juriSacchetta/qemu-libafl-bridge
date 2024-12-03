#include <pth.h>
#include "qemu/osdep.h"

#include "fibers.h"
#include "include/fibers-futex.h"
#include "include/fibers-thread.h"
#include "include/fibers-utils.h"

void fibers_init(void)
{
    fibers_futex_init();
    fibers_thread_init();
}

void fibers_call_scheduler(void)
{
    int available_threads =
        pth_ctrl(PTH_CTRL_GETTHREADS_NEW | PTH_CTRL_GETTHREADS_READY | PTH_CTRL_GETTHREADS_SUSPENDED);
    if (available_threads > 0)
    {
        FIBERS_LOG_DEBUG("Calling scheduler num: %d\n", random_number);
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
qemu_fiber *fiber_stopped = NULL;
extern CPUArchState *libafl_qemu_env;

extern void cpu_loop(CPUArchState *env);

void *fibers_cpu_loop(void *arg)
{
    CPUArchState *env = cpu_env(arg);
    cpu_loop(env);
    fibers_exit(false);
    return NULL;
}

void fibers_save_stopped_thread(CPUState *cpu)
{
    fiber_stopped = fibers_thread_by_pth(pth_self());
    fiber_stopped->stopped = true;
    fiber_stopped->cpu_state = cpu;
}
static int check_exit_condition(void *arg)
{
    return fiber_stopped != NULL;
}

int libafl_qemu_run(void)
{
    if (fiber_stopped != NULL)
    {
        pth_attr_t attr = pth_attr_new();
        pth_attr_set(attr, PTH_ATTR_JOINABLE, 0);
        fiber_stopped->thread = pth_spawn(attr, fibers_cpu_loop, fiber_stopped->cpu_state);
        pth_attr_destroy(attr);
        fiber_stopped->stopped = false;
        fiber_stopped = NULL;
    }
    else
    {
        CPUState *cpu = env_cpu(libafl_qemu_env);
        fibers_spawn_cpu_loop(cpu);
    }
    pth_wait(pth_event(PTH_EVENT_FUNC, check_exit_condition, NULL, pth_time(0, 5000)));
    if(fiber_stopped != NULL)
    {
        pth_abort(fiber_stopped->thread);
    }
    return 0;
}
#endif