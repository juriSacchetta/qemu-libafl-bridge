#include "fibers-types.h"
#include "../fibers.h"
#include "qemu/osdep.h"
#include "qemu/queue.h"

extern int fibers_count;

void fibers_thread_init(void);
void fibers_thread_clear_all(void);
QLIST_HEAD(qemu_fiber_list, qemu_fiber);
extern struct qemu_fiber_list fiber_list_head;
inline qemu_fiber *fibers_thread_by_pth(pth_t thread)
{
    qemu_fiber *current;
    QLIST_FOREACH(current, &fiber_list_head, entry)
    {
        if (current->thread == thread)
            return current;
    }
    return NULL;
}

inline qemu_fiber *fibers_thread_by_tid(int fibers_tid)
{
    qemu_fiber *current;
    QLIST_FOREACH(current, &fiber_list_head, entry)
    {
        if (current->fibers_tid == fibers_tid)
            return current;
    }
    return NULL;
}

#ifdef AS_LIB
void *fibers_cpu_loop(void *arg);
inline qemu_fiber *fibers_spawn_cpu_loop(CPUArchState *cpu)
{
    return fibers_spawn(-1, cpu, fibers_cpu_loop, cpu);
}
#endif