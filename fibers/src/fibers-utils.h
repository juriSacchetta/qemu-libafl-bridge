#pragma once

#include "../pth/pth.h"
#include "fibers-types.h"
#include "qemu/queue.h"

#define DECLARE_FIBER_SYSCALL(type, name, ...) type fibers_syscall_##name(__VA_ARGS__);
#define DEFINE_FIBER_SYSCALL(type, name, ...) type fibers_syscall_##name(__VA_ARGS__) 
#define fiber_syscall(name) fibers_syscall_##name
#define BASE_FIBERS_TID 0x3ffffff

#define FIBERS_LOG_DEBUG(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)