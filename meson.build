project(
    'core-lib',
    'cpp',
    version: '0.1',
    default_options: ['warning_level=3', 'cpp_std=c++23'],
)

catch2_dep = subproject('catch2').get_variable('catch2_with_main_dep')

subdir('src')
subdir('tests')

library(
    'core-lib',
    dependencies: project_lib_dep,
)

test_exe = executable(
    'core-lib-test',
    dependencies: [project_test_dep, catch2_dep],
)

test('test', test_exe, args: ['--order', 'rand', '--rng-seed', 'time', '--warn', 'NoAssertions'])
