#pragma once

#include "../pth/pth.h"
#include "qemu/queue.h"

#define DECLARE_FIBER_SYSCALL(type, name, ...) type fibers_syscall_##name(__VA_ARGS__);
#define DEFINE_FIBER_SYSCALL(type, name, ...) type fibers_syscall_##name(__VA_ARGS__) 
#define fiber_syscall(name) fibers_syscall_##name

#define FIBERS_LOG_DEBUG(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)