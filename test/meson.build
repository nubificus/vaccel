repo_info_script = meson.project_source_root() / 'scripts/get-repo-info.sh'

conf = configuration_data()
conf.set('SOURCE_ROOT', meson.project_source_root())
conf.set('BUILD_ROOT', meson.project_build_root())
conf.set('REPO_URL',
  run_command(repo_info_script, 'url', check: true).stdout().strip())
conf.set('REPO_BRANCH',
  run_command(repo_info_script, 'branch', check: true).stdout().strip())
conf.set('REPO_RAWURL',
  run_command(repo_info_script, 'rawurl', check: true).stdout().strip())

utils_hpp = configure_file(input : 'utils.hpp.in',
  output : 'utils.hpp',
  configuration : conf)

tests_includes = include_directories('.', 'fff', 'catch2')

subdir('core')
subdir('ops')
subdir('utils')

tests_cpp_args = vaccel_cpp_args + [
  '-DCATCH_CONFIG_FAST_COMPILE',
  '-Wno-format-truncation'
]

tests_args = [
  '--order rand',
  '--warn NoAssertions',
]

tests_env = environment()
tests_env.set('VACCEL_LOG_LEVEL', '4')
tests_env.set('VACCEL_PLUGINS','')
tests_env.set('VACCEL_EXEC_DLCLOSE', '1')

tests_env_noop = tests_env
tests_env_noop.set('VACCEL_PLUGINS', libvaccel_noop.full_path())
tests_env_exec = tests_env
tests_env_exec.set('VACCEL_PLUGINS', libvaccel_exec.full_path())

libtests_main = library('tests-main',
  ['main.cpp', 'utils.cpp', utils_hpp, 'mock_virtio.cpp', 'mock_virtio.hpp'],
  cpp_args : tests_cpp_args,
  include_directories : tests_includes,
  dependencies : libvaccel_internal_dep,
  install : false)

foreach t : tests_core_sources + tests_utils_sources
  name = fs.stem(t)
  exe = executable(name,
    sources : t,
    cpp_args : tests_cpp_args,
    include_directories : tests_includes,
    dependencies : libvaccel_internal_dep,
    link_with : libtests_main,
    install : false)

  if name == 'test_core'
    tests_env_core = tests_env
    tests_env_core.set('VACCEL_BOOTSTRAP_ENABLED', '0')
    tests_env_core.set('VACCEL_CLEANUP_ENABLED', '0')
    test(name, exe,
      args : tests_args,
      env : tests_env_core,
      is_parallel : false)
    continue
  endif
  test(name, exe, args : tests_args, env : tests_env, is_parallel : false)
  if name == 'test_log'
    foreach l : range(1, 4)
      tests_env_log = tests_env
      tests_env_log.set('VACCEL_LOG_LEVEL', l.to_string())
      test('@0@+@1@@2@'.format(name, 'level', l), exe,
        args : tests_args,
        env : tests_env_log,
        is_parallel : false)
    endforeach
  endif
endforeach

foreach t : tests_ops_sources
  name = fs.stem(t)
  exe = executable(name,
    sources : t,
    cpp_args : tests_cpp_args,
    include_directories : tests_includes,
    dependencies : libvaccel_internal_dep,
    link_with : libtests_main,
    install : false)

  test(name + '+noop', exe,
    args : tests_args,
    env : tests_env_noop,
    is_parallel : false)

  if name.contains('exec')
    test(name + '+exec', exe,
      args : tests_args,
      env : tests_env_exec,
      is_parallel : false)
  endif
endforeach

gcovr_cfg = '''
To enable coverage measurements configure with option:
-Db_coverage=true

and create a gcovr.cfg in root dir with:
gcov-executable = gcov-@0@
filter = src/
filter = plugins/

For more info see:
https://mesonbuild.com/Unit-tests.html#coverage
https://mesonbuild.com/howtox.html#producing-a-coverage-report
https://gcovr.com/en/stable/guide/configuration.html
'''
message(gcovr_cfg.format(cpp.version().split('.')[0]))
