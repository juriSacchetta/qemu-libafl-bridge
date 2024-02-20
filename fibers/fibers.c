#include "fibers.h"

#include "pth/pth.h"
#include "qemu/osdep.h"
#include "src/fibers-futex.h"
#include "src/fibers-thread.h"
#include "src/fibers-utils.h"

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
extern __thread CPUArchState *libafl_qemu_env;

extern void cpu_loop(CPUArchState *env);

static void *fibers_cpu_loop(void *arg)
{
    cpu_loop((CPUArchState *)arg);
    fibers_unregister_thread(pth_self());
    pth_exit(NULL);
    return NULL;
}

void fibers_save_stopped_thread(CPUArchState *cpu)
{
    fiber_stopped = fibers_thread_by_pth(pth_self());
    fiber_stopped->stopped = true;
    fiber_stopped->env = cpu;
}
static int check_exit_condition(void *arg)
{
    return fiber_stopped != NULL;
}

int libafl_qemu_run(void)
{
    if (fiber_stopped != NULL)
    {
        fiber_stopped->stopped = false;
        fiber_stopped->thread =
            pth_spawn(PTH_ATTR_DEFAULT, env_cpu(fiber_stopped->env), fibers_cpu_loop, fiber_stopped->env);
        fiber_stopped = NULL;
    }
    else
    {
        fibers_register_thread(pth_spawn(PTH_ATTR_DEFAULT, env_cpu(libafl_qemu_env), fibers_cpu_loop, libafl_qemu_env),
                               libafl_qemu_env);
    }
    pth_wait(pth_event(PTH_EVENT_FUNC, check_exit_condition, NULL, pth_time(0, 500000)));
    return 0;
}
#endif