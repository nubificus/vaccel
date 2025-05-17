project('vaccel', 'c', 'cpp',
  meson_version : '>=1.1',
  default_options : ['c_std=c11'],
  license : 'Apache-2.0',
  license_files : 'LICENSE',
  version : run_command('sh', '-c',
    'git submodule update --init >/dev/null && ' +
    'scripts/common/generate-version.sh .',
    check: true).stdout().strip())

vaccel_common_args = ['-Wall','-Wextra','-Werror']
vaccel_c_args = vaccel_common_args
vaccel_cpp_args = vaccel_common_args

libvaccel_version = meson.project_version().split('-')[0]

opt_tests = get_option('tests')
opt_examples = get_option('examples').enable_if(opt_tests.enabled())

slog_subproj = subproject('slog')
libslog_dep = dependency('slog')

libcurl_dep = dependency('libcurl', required : false)
if libcurl_dep.found()
  vaccel_c_args += '-DUSE_LIBCURL'
  vaccel_cpp_args += '-DUSE_LIBCURL'
endif

cc = meson.get_compiler('c')
cpp = meson.get_compiler('cpp')
if cc.has_header('stb/stb_image.h')
  vaccel_c_args += '-DUSE_STB_IMAGE'
endif
if cpp.has_header('stb/stb_image.h')
  vaccel_cpp_args += '-DUSE_STB_IMAGE'
endif

libm_dep = cc.find_library('m')

subdir('src')
subdir('plugins')
subdir('python')

fs = import('fs')

if opt_examples.enabled()
  subdir('examples')
endif

if opt_tests.enabled()
  subdir('test')
endif

summary({
  'Build the exec plugin': opt_plugin_exec.enabled(),
  'Build the no-op debugging plugin': opt_plugin_noop.enabled(),
  'Build the mbench plugin': opt_plugin_mbench.enabled(),
  'Build the examples': opt_examples.enabled(),
  'Enable testing': opt_tests.enabled(),
  },
  section : 'Configuration',
  bool_yn : true)

meson.add_dist_script(
  'scripts/common/dist.sh',
  '-n', meson.project_name(),
  '-v', meson.project_version(),
  '-t', get_option('buildtype'),
  '-a', 'plugin-exec=' + (opt_plugin_exec.enabled() ? 'enabled' : 'disabled'),
  '-a', 'plugin-noop=' + (opt_plugin_noop.enabled() ? 'enabled' : 'disabled'),
  '-a', 'plugin-mbench=' + (opt_plugin_mbench.enabled() ? 'enabled' : 'disabled'),
  '-a', 'examples=' + (opt_examples.enabled() ? 'enabled' : 'disabled'),
  '-a', 'tests=' + (opt_tests.enabled() ? 'enabled' : 'disabled'))

if opt_examples.enabled() and opt_plugin_noop.enabled() and opt_plugin_exec.enabled()
  run_target('run-examples',
    command : ['scripts/run-examples.sh', meson.project_name()],
    env : {'PKG_CONFIG_PATH': get_option('pkg_config_path')},
    depends : [examples, libvaccel_noop, libvaccel_exec])
  run_target('run-examples-valgrind',
    command : ['scripts/run-examples.sh', meson.project_name(), '1'],
    env : {'PKG_CONFIG_PATH': get_option('pkg_config_path')},
    depends : [examples, libvaccel_noop, libvaccel_exec])
endif

