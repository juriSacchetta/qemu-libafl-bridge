project('pth', 'c')

# Specify the minimum required version of Meson
meson_version = '>=0.58.0'

# Use the 'configure_file' function to generate a config.h file
# configure_file(
#   output: 'config.h',
#   configuration: 'config.h.in',
# )

# Add your source files here
pth_sources = files(
    'pth_acmac.h',
    'pth_attr.c',
    'pth_acdef.h',
    'pth_cancel.c',
    'pth_clean.c',
    'pth_compat.c',
    'pth_data.c',
    'pth_debug.c',
    'pth_errno.c',
    'pth_event.c',
    'pth_ext.c',
    'pth_fork.c',
    'pth_high.c',
    'pth_lib.c',
    'pth_mctx.c',
    'pth_msg.c',
    'pth_p.h',
    'pth_pqueue.c',
    'pth_ring.c',
    'pth_sched.c',
    'pth_string.c',
    'pth_sync.c',
    'pth_syscall.c',
    'pth_tcb.c',
    'pth_time.c',
    'pth_uctx.c',
    'pth_util.c',
    'pth_vers.c',
    'include/pth.h',
)

# Create a shared library target
pth_lib = library(
  'pth',
  pth_sources,
)

pth_dep = declare_dependency(link_with : pth_lib, include_directories : include_directories(['include']))


# Add additional dependencies if needed
# For example, if Pth depends on another library like 'libfoo':
# libfoo_dep = dependency('libfoo')

