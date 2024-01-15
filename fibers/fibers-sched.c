
#include "pth/pth.h"
#include "fibers-sched.h"

void inline fibers_call_scheduler(void) {
    pth_yield(NULL);
}