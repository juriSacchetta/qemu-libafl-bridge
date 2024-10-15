#pragma once

#include <pth.h>
#include "fibers-types.h"
#include "qemu/queue.h"

#define GEN_SYSCALL_WRAPPER_SIGN(type, name, ...) type fibers_syscall_##name(__VA_ARGS__);

// Uncomment the line below to enable debug output
//#define FIBER_DEBUG
#ifdef FIBER_DEBUG
#define FIBERS_LOG_DEBUG(fmt, ...) \
    do { qemu_log("QEMU_FIBERS (DEBUG): " fmt, ##__VA_ARGS__); } while (0)
#else
#define FIBERS_LOG_DEBUG(fmt, ...) \
    do { } while (0)
#endif

//#define FIBER_INFO
#ifdef FIBER_INFO
#define FIBERS_LOG_INFO(fmt, ...) \
    do { qemu_log("QEMU_FIBERS (INFO): " fmt, ##__VA_ARGS__); } while (0)
#else
#define FIBERS_LOG_INFO(fmt, ...) \
    do { } while (0)
#endif

#define BASE_FIBERS_TID 0x3ffffff