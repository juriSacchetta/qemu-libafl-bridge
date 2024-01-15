#include "qemu/osdep.h"
#include "src/fibers-thread.h"
#include "fibers.h"

void fibers_init(CPUArchState *cpu)
{
   fibers_thread_init(cpu);
}

