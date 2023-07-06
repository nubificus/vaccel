project('vaccel', 'c', 'cpp',
  meson_version : '>=1.1',
  default_options : ['c_std=c11'],
  license : 'Apache-2.0',
  license_files : 'LICENSE',
  version : run_command('scripts/generate-version.sh',
    check: false).stdout().strip())

vaccel_cargs = ['-Wall','-Wextra','-Werror']
plugins_cargs = ['-Wno-unused']

vaccel_version = run_command('scripts/generate-version.sh',
  meson.project_source_root(),
  check: false).stdout().strip()

libvaccel_version = vaccel_version.split('-')[0]

meson.add_dist_script('scripts/dist.sh')
opt_tests = get_option('tests')
opt_examples = get_option('examples').enable_if(opt_tests.enabled())

slog_subproj = subproject('slog')
#libslog_dep = slog_subproj.get_variable('libslog_dep')
libslog_dep = dependency('slog')

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
  'Build the exec plugin': opt_plugin_exec,
  'Build the no-op debugging plugin': opt_plugin_noop,
  'Build the examples': opt_examples,
  'Enable testing': opt_tests,
  },
  section : 'Configuration',
  bool_yn : true)
