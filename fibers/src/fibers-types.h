#pragma once 

#include "qemu/osdep.h"
#include "qemu/queue.h"
#include <pth.h>


typedef struct qemu_fiber{
    CPUArchState *env;
    int fibers_tid;
    pth_t thread;
    QLIST_ENTRY(qemu_fiber) entry;
} qemu_fiber;


