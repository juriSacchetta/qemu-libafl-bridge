#include "fibers-types.h"
#include "../fibers.h"
#include "qemu/osdep.h"
#include "qemu/queue.h"

extern int fibers_count;

void fibers_thread_init(CPUState *env_cpu);
void fibers_thread_clear_all(void);
QLIST_HEAD(qemu_fiber_list, qemu_fiber);

extern struct qemu_fiber_list fiber_list_head;
qemu_fiber *fibers_thread_by_pth(pth_t thread);
qemu_fiber *fibers_thread_by_tid(int fibers_tid);

#ifdef AS_LIB
void *fibers_cpu_loop(void *arg);
inline qemu_fiber *fibers_spawn_cpu_loop(CPUArchState *cpu)
{
    return fibers_spawn(-1, cpu, fibers_cpu_loop, cpu);
}
#endif