#pragma once

#include <pth.h>
#include "qemu/osdep.h"
#include "qemu/queue.h"

typedef struct qemu_fiber
{
    CPUState *cpu_state;
    int fibers_tid;
    pth_t thread;
    QLIST_ENTRY(qemu_fiber) entry;
#ifdef AS_LIB
    bool stopped;
#endif
} qemu_fiber;
