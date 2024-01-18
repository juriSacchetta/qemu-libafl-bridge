#include "fibers-types.h"
#include "qemu/osdep.h"

extern int fibers_count;

void fibers_thread_init(CPUArchState *cpu);
qemu_fiber * fibers_thread_by_pth(pth_t thread);
qemu_fiber * fibers_thread_by_tid(int fibers_tid);