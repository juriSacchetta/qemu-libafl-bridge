#include "qemu/queue.h"
#include "src/fibers-types.h"
#include "src/fibers-thread.h"
#include "fibers.h"

struct qemu_fiber_list fiber_list_head;
int fibers_count = BASE_FIBERS_TID;

void fibers_thread_init(CPUState *cpu)
{
    QLIST_INIT(&fiber_list_head);
    qemu_fiber *main = malloc(sizeof(qemu_fiber));
    memset(main, 0, sizeof(qemu_fiber));
    QLIST_INSERT_HEAD(&fiber_list_head, main, entry);
    main->fiber_tid = fibers_count;
    main->thread = pth_init(cpu);
}

qemu_fiber *fiber_spawn(int tid, CPUArchState *cpu, void *(*func)(void *), void *arg)
{
    qemu_fiber *new = malloc(sizeof(qemu_fiber));
    memset(new, 0, sizeof(qemu_fiber));

    pth_attr_t attr = pth_attr_new();
    pth_attr_set(attr, PTH_ATTR_JOINABLE, 0);

    new->thread = pth_spawn(attr, env_cpu(cpu), func, arg);

    pth_attr_destroy(attr);

    if (tid == -1)
    {
        new->fiber_tid = ++fibers_count;
    }
    else
    {
        new->fiber_tid = tid;
    }

    new->env = cpu;

    QLIST_INSERT_HEAD(&fiber_list_head, new, entry);
    return new;
}

void fiber_exit(bool continue_execution)
{
    qemu_fiber *fiber = fiber_by_pth(pth_self());
    assert(fiber != NULL);
#ifdef AS_LIB
    if (!fiber->stopped)
    {
#endif
        QLIST_REMOVE(fiber, entry);
        free(fiber);
#ifdef AS_LIB
    }
#endif
    if (!continue_execution)
        pth_exit(NULL);
}

void fibers_thread_clear_all(void)
{
    qemu_fiber *current;
    QLIST_FOREACH(current, &fiber_list_head, entry)
    {
        if (current->thread == pth_self())
            continue;
        #ifdef AS_LIB
        if (current->stopped == true)
            continue;
        #endif
        QLIST_REMOVE(current, entry);
        pth_abort(current->thread);
        free(current);
    }
}

qemu_fiber *fiber_by_pth(pth_t thread)
{
    qemu_fiber *current;
    QLIST_FOREACH(current, &fiber_list_head, entry)
    {
        if (current->thread == thread)
            return current;
    }
    return NULL;
}

qemu_fiber *fiber_by_tid(int fiber_tid)
{
    qemu_fiber *current;
    QLIST_FOREACH(current, &fiber_list_head, entry)
    {
        if (current->fiber_tid == fiber_tid)
            return current;
    }
    return NULL;
}


#ifdef AS_LIB
void fiber_restore_thread(int tid, CPUArchState *s)
{
    qemu_fiber *current = fiber_by_tid(tid);
    if (current != NULL)
    {
        current->env = s;
        return;
    }
    fiber_spawn(tid, s, fibers_cpu_loop, s);
}
#endif