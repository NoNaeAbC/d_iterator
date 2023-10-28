project('header-only-lib', 'cpp',
    version : '0.1',
    default_options : ['warning_level=3', 'b_lto=true','cpp_std=c++20', 'werror=true', 'buildtype=debugoptimized']
)
gtest_src = subproject('gtest', required: true).get_variable('gtest_dep')

header_only_lib = declare_dependency(
    include_directories : 'include'
)

asan_flags = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']

unit_test_exe = executable(
    'unit_test',
    'test/unit_test.cpp',
    dependencies : [header_only_lib, gtest_src],
    link_args : asan_flags,
    cpp_args : asan_flags
)

test('unit_test', unit_test_exe)

# Syntax-only checks for -Wall -Wextra -Werror with GCC and Clang
compilers_to_check = ['gcc', 'clang']

foreach cc : compilers_to_check
    if cc == 'gcc'
        compiler_path = 'g++'
    elif cc == 'clang'
        compiler_path = 'clang++'
    endif

    run_target(
        'syntax-check-' + cc,
        command : [compiler_path, '-fsyntax-only', '-std=c++20', '-Wall', '-Wextra', '-Werror', meson.current_source_dir() + '/test/unit_test.cpp', '-I' + meson.current_source_dir() + '/include'],
        depends : unit_test_exe
    )
endforeach

# clang-tidy and cppcheck
clangtidy = find_program('clang-tidy', required : false)
cppcheck = find_program('cppcheck', required : false)

if clangtidy.found()
    run_target('clang-tidy',
        command : [clangtidy, '-p', meson.current_build_dir(), meson.current_source_dir() + '/test/unit_test.cpp','--warnings-as-errors=*', '--header-filter=.*', '--checks=bugprone-*,performance-*,readability-*,clang-analyzer-*,-readability-identifier-length,misc-const-correctness']
    )
endif

if cppcheck.found()
    run_target('cppcheck',
        command : [cppcheck, '--check-level=exhaustive','--enable=warning,performance,portability,style', '--std=c++20',  meson.current_source_dir() + '/test/unit_test.cpp', '-I' + meson.current_source_dir() + '/include']
    )
endif

compiler = meson.get_compiler('cpp')

cpu_family = host_machine.cpu_family()
message('Compiler ID: ' + compiler.get_id())
native_flags = []

# Check for GCC or Clang on x86 or x86_64
if (compiler.get_id() == 'gcc' or compiler.get_id() == 'clang') and (cpu_family == 'x86' or cpu_family == 'x86_64')
    native_flags = ['-march=native']
elif compiler.get_id() == 'gcc' or compiler.get_id() == 'clang'
    native_flags = ['-mcpu=native']
elif compiler.get_id() == 'msvc'
    native_flags = ['/arch:AVX2']
  # Here, you can add logic to set specific architecture flags for MSVC, if needed.
  # MSVC doesn't have a direct equivalent to `-march=native`, so you might need to determine the proper flag based on the host or leave it as is.
endif

benchmark_dep = dependency('benchmark')
threads_dep = dependency('threads')
benchmark_exe = executable(
    'benchmark_exe',
    'bench/benchmark.cpp',
    dependencies: [benchmark_dep, threads_dep],
    cpp_args: ['-Ofast'] + native_flags,
    build_by_default: false  # Prevents building the benchmark every time you run ninja.
)
