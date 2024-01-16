#include "qemu/osdep.h"
#include "src/fibers-thread.h"
#include "pth/pth.h"

#include "fibers.h"

void fibers_init(CPUArchState *cpu)
{
   fibers_thread_init(cpu);
}

void fibers_call_scheduler(void)
{
   if (fibers_count - BASE_FIBERS_TID > 1) {
      pth_yield(NULL);
   }
}