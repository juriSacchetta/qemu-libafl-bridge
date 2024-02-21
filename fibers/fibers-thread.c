#include "src/fibers-thread.h"
#include "fibers.h"
#include "qemu/queue.h"
#include "src/fibers-types.h"

struct qemu_fiber_list fiber_list_head;
int fibers_count = BASE_FIBERS_TID;

void fibers_thread_init(void)
{
    QLIST_INIT(&fiber_list_head);
    qemu_fiber *main = malloc(sizeof(qemu_fiber));
    memset(main, 0, sizeof(qemu_fiber));
    QLIST_INSERT_HEAD(&fiber_list_head, main, entry);
    main->fibers_tid = fibers_count;
    main->thread = pth_init(NULL);
}

qemu_fiber *fibers_spawn(pth_attr_t attr, int tid, CPUArchState *cpu, void *(*func)(void *), void *arg)
{
    qemu_fiber *new = malloc(sizeof(qemu_fiber));
    memset(new, 0, sizeof(qemu_fiber));

    if (attr == NULL)
    {
        attr = PTH_ATTR_DEFAULT;
        pth_attr_set(attr, PTH_ATTR_JOINABLE, 0);
    }
    new->thread = pth_spawn(attr, env_cpu(cpu), func, arg);

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
    fibers_spawn(NULL, tid, s, fibers_cpu_loop, s);
}
