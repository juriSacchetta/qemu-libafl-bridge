#include "include/fibers-thread.h"
#include "fibers.h"
#include "qemu/queue.h"
#include "include/fibers-types.h"


struct qemu_fiber_list fiber_list_head;
int fibers_count = BASE_FIBERS_TID;

void fibers_thread_init(void)
{
    QLIST_INIT(&fiber_list_head);
    qemu_fiber *main = malloc(sizeof(qemu_fiber));
    memset(main, 0, sizeof(qemu_fiber));
    QLIST_INSERT_HEAD(&fiber_list_head, main, entry);
    main->fibers_tid = fibers_count;
    if(!pth_init())
    {
        //FIXME: Pass to use qemu_log (log.h)
        fprintf(stderr, "PTH init failed\n");
        exit(1);
    }
    //FIXME: main->thread = pth_init();
}

qemu_fiber *fibers_spawn(int tid, CPUArchState *cpu, void *(*func)(void *), void *arg)
{
    qemu_fiber *new = malloc(sizeof(qemu_fiber));
    memset(new, 0, sizeof(qemu_fiber));

    pth_attr_t attr = pth_attr_new();
    pth_attr_set(attr, PTH_ATTR_JOINABLE, 0);

    new->thread = pth_spawn(attr, func, arg);

    pth_attr_destroy(attr);

    if (tid == -1)
    {
        new->fibers_tid = ++fibers_count;
    }
    else
    {
        new->fibers_tid = tid;
    }

    new->env = cpu;

    QLIST_INSERT_HEAD(&fiber_list_head, new, entry);
    return new;
}

void fibers_exit(bool continue_execution)
{
    qemu_fiber *fiber = fibers_thread_by_pth(pth_self());
    assert(fiber != NULL);
    if (!fiber->stopped)
    {
        QLIST_REMOVE(fiber, entry);
        free(fiber);
    }
    if (!continue_execution)
        pth_exit(NULL);
}

void fibers_thread_clear_all(void)
{
    qemu_fiber *current;
    QLIST_FOREACH(current, &fiber_list_head, entry)
    {
        if (current->thread == pth_self() || current->stopped == true)
            continue;
        QLIST_REMOVE(current, entry);
        pth_abort(current->thread);
        free(current);
    }
}

void fibers_restore_thread(int tid, CPUArchState *s)
{
    qemu_fiber *current = fibers_thread_by_tid(tid);
    if (current != NULL)
    {
        current->env = s;
        return;
    }
    fibers_spawn(tid, s, fibers_cpu_loop, s);
}

// static void fibers_thread_print_all(void) __attribute__((used));
// static void fibers_thread_print_all(void)
// {
//     qemu_fiber *current;
//     int count = 0;
//     fprintf(stderr, "!!!!!!!!!!!!Fibers!!!!!!!!!!!!!!\n");
//     QLIST_FOREACH(current, &fiber_list_head, entry)
//     {
//         if (current->env != NULL)
//         {
//             fprintf(stderr, "Fiber   %d\n", current->fibers_tid);
//             fprintf(stderr, "Thread  %p\n", current->thread);
//             fprintf(stderr, "CPU     %p\n", current->env);
//         }
//         else
//         {
//             fprintf(stderr, "Main thread\n");
//             fprintf(stderr, "Fiber   %d\n", current->fibers_tid);
//             fprintf(stderr, "Thread  %p\n", current->thread);
//             fprintf(stderr, "CPU     %p\n", current->env);
//         }
//         fprintf(stderr, "\n");

//         count++;
//     }
//     fprintf(stderr, "Fibers count:   %d\n", count);
//     // PTH_CTRL_GETTHREADS_NEW for the number of threads in the new queue (threads created via pth_spawn(3) bu\t still not scheduled once), PTH_CTRL_GETTHREADS_READY for the number of threads in the ready queue (threads who want to do CPU bursts), PTH_CTRL_GETTHREADS_RUNNING for the number of running threads (always just one thread!), PTH_CTRL_GETTHREADS_WAITING for the number of threads in the waiting queue (threads waiting for events), PTH_CTRL_GETTHREADS_SUSPENDED for the number of threads in the suspended queue (threads waiting to be resumed) and PTH_CTRL_GETTHREADS_DEAD for the number of threads in the new queue (terminated threads waiting for a join).
//     fprintf(stderr, "Pth count:      %ld\n", pth_ctrl(PTH_CTRL_GETTHREADS));
//     fprintf(stderr, "Pth dead threads:      %ld\n", pth_ctrl(PTH_CTRL_GETTHREADS_DEAD));
//     fprintf(stderr, "Pth new threads:      %ld\n", pth_ctrl(PTH_CTRL_GETTHREADS_NEW));
//     fprintf(stderr, "Pth ready threads:      %ld\n", pth_ctrl(PTH_CTRL_GETTHREADS_READY));
//     fprintf(stderr, "Pth running threads:      %ld\n", pth_ctrl(PTH_CTRL_GETTHREADS_RUNNING));
//     fprintf(stderr, "Pth waiting threads:      %ld\n", pth_ctrl(PTH_CTRL_GETTHREADS_WAITING));
//     fprintf(stderr, "Pth suspended threads:      %ld\n", pth_ctrl(PTH_CTRL_GETTHREADS_SUSPENDED));
//     fprintf(stderr, "!!!!!!!!!!!!Fibers!!!!!!!!!!!!!!\n");
//     fflush(stderr);
// }
