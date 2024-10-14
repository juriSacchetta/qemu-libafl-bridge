if not have_qemu_fibers
   subdir_done()
endif

fibers_ss = ss.source_set()

fibers_ss.add(
    files(  'fibers.c',
            'fibers-futex.c',
            'fibers-syscall.c',
            'fibers-thread.c')
)

pth_dep = dependency('pth', required: false)

if not pth_dep.found()
   run_command('sh', '-c', 'echo "Library not found, installing..."; ./pth/install_pth.sh')
   pth_dep = declare_dependency(
      include_directories: include_directories('./pth/install/include'),
      link_args: ['-L./pth/install/lib', '-lpth']
   )
endif


fibers_ss.add(pth_dep)

specific_ss.add_all(fibers_ss)