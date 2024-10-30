#ifndef QEMU_THREAD_POSIX_H
#define QEMU_THREAD_POSIX_H

#ifndef QEMU_FIBERS
#include <pthread.h>
#include <semaphore.h>
#else
#include "fibers/pth/pth.h"
#endif

struct QemuMutex {
#ifndef QEMU_FIBERS
    pthread_mutex_t lock;
#else
    pth_mutex_t lock;
#endif
#ifdef CONFIG_DEBUG_MUTEX
    const char *file;
    int line;
#endif
    bool initialized;
};

/*
 * QemuRecMutex cannot be a typedef of QemuMutex lest we have two
 * compatible cases in _Generic.  See qemu/lockable.h.
 */
typedef struct QemuRecMutex {
    QemuMutex m;
} QemuRecMutex;

struct QemuCond {
#ifndef QEMU_FIBERS
    pthread_cond_t cond;
#else
    pth_cond_t cond;
#endif
    bool initialized;
};

struct QemuSemaphore {
    QemuMutex mutex;
    QemuCond cond;
    unsigned int count;
};

struct QemuEvent {
#ifndef __linux__
    pthread_mutex_t lock;
    pthread_cond_t cond;
#endif
    unsigned value;
    bool initialized;
};

struct QemuThread {
#ifndef QEMU_FIBERS
    pthread_t thread;
#else
    pth_t thread;
    
#endif
};

#endif
