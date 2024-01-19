#include "qemu/osdep.h"
#include "pth/pth.h"
#include "unistd.h"

#include "fibers.h"
#include "src/fibers-thread.h"
#include "src/fibers-futex.h"
#include "src/fibers-utils.h"

void fibers_init(CPUArchState *cpu)
{
   srandom(2000);
   fibers_futex_init();
   fibers_thread_init(cpu);
}

void fibers_call_scheduler(void)
{
   int available_threads = pth_ctrl(PTH_CTRL_GETTHREADS_NEW | PTH_CTRL_GETTHREADS_READY | PTH_CTRL_GETTHREADS_SUSPENDED);
   if (available_threads > 0) {
      int random_number = random() % 2;
      if(random_number < 1) {
         FIBERS_LOG_DEBUG("Calling scheduler num: %d\n", random_number);
         pth_yield(NULL);
      }
   }
}

void fibers_fork_end(bool child) {
   if(child) {
      fibers_thread_clear_all();
      fibers_clean_futex();
   }
}