subdir('error')
subdir('ringbuf')
subdir('panic')

project_lib_dep = declare_dependency(
    include_directories: '.',
    sources: files(),
    dependencies: [error_dep, ringbuf_dep, panic_dep],
)
