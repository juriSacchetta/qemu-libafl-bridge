#include "../fibers.h"
#include "qemu/osdep.h"

#ifdef AS_LIB
void *fibers_cpu_loop(void *arg);
void fiber_restore_thread(int tid, CPUArchState *s);
inline qemu_fiber *fiber_spawn_cpu_loop(CPUArchState *cpu)
{
    return fiber_spawn(-1, cpu, fibers_cpu_loop, cpu);
}
#endif