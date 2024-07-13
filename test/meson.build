compiler = meson.get_compiler('cpp')
if not compiler.has_argument('-std=c++2b')
  error('A compiler supporting -std=c++2b is required to build the tests')
endif

conf = configuration_data()
conf.set('SOURCE_ROOT', meson.project_source_root())
conf.set('BUILD_ROOT', meson.project_build_root())

utils_hpp = configure_file(input : 'utils.hpp.in',
  output : 'utils.hpp',
  configuration : conf)

tests_includes = include_directories('.', 'fff', 'catch2')

subdir('core')
subdir('ops')
subdir('resources')

libtests_main = library('tests-main',
  ['main.cpp', 'utils.cpp', vaccel_utils_c],
  cpp_args : vaccel_cargs,
  include_directories : [tests_includes, vaccel_src_includes],
  dependencies : libvaccel_private,
  install : false)


tests_cpp_args = [
  '-std=c++2b',
  '-DCATCH_CONFIG_FAST_COMPILE'
]

tests_args = [
  '--order rand',
  '--warn NoAssertions',
]

tests_env = [
  'VACCEL_DEBUG_LEVEL=4',
]

tests_env_noop = 'VACCEL_BACKENDS=' + libvaccel_noop.full_path()
tests_env_exec = 'VACCEL_BACKENDS=' + libvaccel_exec.full_path()

foreach t : tests_core_sources
  name = fs.stem(t)
  exe = executable(name,
    sources : t,
    cpp_args : vaccel_cargs + tests_cpp_args,
    include_directories : [tests_includes, vaccel_src_includes],
    dependencies : libvaccel_private,
    link_with : libtests_main,
    install : false)

  test(name, exe, args : tests_args, env : tests_env, is_parallel : false)
  if name == 'test_log'
    foreach l : range(1, 4)
      test('@0@+@1@@2@'.format(name, 'level', l), exe,
        args : tests_args,
        env : 'VACCEL_DEBUG_LEVEL=' + l.to_string(),
        is_parallel : false)
    endforeach
  endif
endforeach

foreach t : tests_ops_sources + tests_res_sources
  name = fs.stem(t)
  exe = executable(name,
    sources : t,
    cpp_args : vaccel_cargs + tests_cpp_args,
    include_directories : [tests_includes, vaccel_src_includes],
    dependencies : libvaccel_private,
    link_with : libtests_main,
    install : false)

  test(name + '+noop', exe,
    args : tests_args,
    env : tests_env + tests_env_noop,
    is_parallel : false)

  if name.contains('exec')
    test(name + '+exec', exe,
      args : tests_args,
      env : tests_env + tests_env_exec + 'VACCEL_EXEC_DLCLOSE=1',
      is_parallel : false)
  endif
endforeach

gcovr_cfg = '''
To enable coverage measurements configure with option:
-Db_coverage=true

and create a gcovr.cfg in root dir with:
gcov-executable = gcov-@0@
filter = ../src/
filter = ../plugins/

For more info see:
https://mesonbuild.com/Unit-tests.html#coverage
https://mesonbuild.com/howtox.html#producing-a-coverage-report
https://gcovr.com/en/stable/guide/configuration.html
'''
message(gcovr_cfg.format(compiler.version().split('.')[0]))
