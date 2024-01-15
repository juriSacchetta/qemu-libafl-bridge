
#include "pth/pth.h"
#include "fibers.h"
void fibers_call_scheduler(void) {
    pth_yield(NULL);
}