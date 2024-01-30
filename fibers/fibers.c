#include "qemu/osdep.h"
#include "pth/pth.h"

#include "fibers.h"
#include "src/fibers-thread.h"
#include "src/fibers-futex.h"
#include "src/fibers-utils.h"

void fibers_init(void)
{
   fibers_futex_init();
   fibers_thread_init();
}

void fibers_call_scheduler(void)
{
   int available_threads = pth_ctrl(PTH_CTRL_GETTHREADS_NEW | PTH_CTRL_GETTHREADS_READY | PTH_CTRL_GETTHREADS_SUSPENDED);
   if (available_threads > 0) {
      FIBERS_LOG_DEBUG("Calling scheduler num: %d\n", random_number);
      pth_yield(NULL);
   }
}

void fibers_fork_end(bool child) {
   if(child) {
      fibers_thread_clear_all();
      fibers_clean_futex();
   }
}

#ifdef AS_LIB
struct fibers_qemu_user_init_params {
    int argc;
    char **argv;
    char **envp;
};

bool exited = false;
CPUArchState *exited_cpu = NULL;
extern __thread CPUArchState *libafl_qemu_env;

extern void cpu_loop(CPUArchState *env);

static void *fibers_cpu_loop(void *arg) {
   CPUArchState *env = (CPUArchState *)arg;
   cpu_loop(env);

   //never reached
   return NULL;
}

void fibers_save_exit_reason(CPUArchState *cpu){
   exited=true;
   exited_cpu = cpu;
   fibers_unregister_thread(pth_self());
}
static int check_exit_condition(void *arg) {
   return exited;
}

int libafl_qemu_run(void) {
   if(exited){
      assert(exited_cpu);
      exited=false;
      fibers_register_thread(pth_spawn(PTH_ATTR_DEFAULT, env_cpu(exited_cpu), fibers_cpu_loop, exited_cpu), exited_cpu); 
   } else {
      fibers_register_thread(pth_spawn(PTH_ATTR_DEFAULT, env_cpu(libafl_qemu_env), fibers_cpu_loop, libafl_qemu_env), libafl_qemu_env);
   }
   pth_wait(pth_event(PTH_EVENT_FUNC, check_exit_condition, NULL, pth_time(0, 500000)));
   return 0;
}


void fibers_register_env(CPUArchState *env) {
   pth_register_env(env_cpu(env));
}

#endif