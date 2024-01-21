if not have_qemu_fibers
   subdir_done()
endif

fibers_ss = ss.source_set()

subdir('pth')

fibers_ss.add(
    files(  'fibers.c',
            'fibers-futex.c',
            'fibers-syscall.c',
            'fibers-thread.c')
)

specific_ss.add_all(fibers_ss)