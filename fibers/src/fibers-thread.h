#include "fibers-types.h"
#include "qemu/osdep.h"

extern int fibers_count;

void fibers_thread_init(CPUState *cpu);
void fibers_thread_clear_all(void);
qemu_fiber *fiber_by_pth(pth_t thread);
qemu_fiber *fiber_by_tid(int fiber_tid);

QLIST_HEAD(qemu_fiber_list, qemu_fiber);
extern struct qemu_fiber_list fiber_list_head;

#ifdef AS_LIB
void *fibers_cpu_loop(void *arg);
inline qemu_fiber *fibers_spawn_cpu_loop(CPUArchState *cpu)
{
    return fibers_spawn(-1, cpu, fibers_cpu_loop, cpu);
}
#endif