#include "qemu/queue.h"
#include "src/fibers-thread.h"
#include "fibers.h"


void fiber_exit(bool continue_execution)
{
    if (!continue_execution)
        pth_exit(NULL);
}

#ifdef AS_LIB
void fiber_restore_thread(int tid, CPUArchState *s)
{
    qemu_fiber *current = fiber_by_tid(tid);
    if (current != NULL)
    {
        current->env = s;
        return;
    }
    fiber_spawn(tid, s, fibers_cpu_loop, s);
}
#endif