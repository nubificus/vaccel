conf = configuration_data()
conf.set('VACCEL_VERSION', meson.project_version())

vaccel_h = configure_file(input : 'vaccel.h.in',
  output : 'vaccel.h',
  configuration : conf)

vaccel_headers = files([
  'utils.h',
  'error.h',
  'log.h',
  'misc.h',
  'plugin.h',
  'session.h',
  'list.h',
  'id_pool.h',
  'resources.h',
  'vaccel_args.h',
])

vaccel_utils_c = files('utils.c')
vaccel_plugin_c = files('plugin.c')
vaccel_sources = files([
  'log.c',
  'misc.c',
  'session.c',
  'resources.c',
  'vaccel.c',
  'id_pool.c',
  'utils.c',
  'vaccel_args.c',
])

subdir('include')
subdir('resources')
subdir('ops')
subdir('profiling')

vaccel_sources += [
  vaccel_plugin_c, vaccel_utils_c,
  vaccel_headers, vaccel_h]
vaccel_includes = include_directories('include')
vaccel_src_includes = include_directories('.')

threads_dep = dependency('threads')
libvaccel_deps = [
  threads_dep,
  dependency('dl'),
  libslog_dep,
]
libvaccel = library('vaccel',
  vaccel_sources,
  version: libvaccel_version,
  c_args : vaccel_cargs,
  include_directories : vaccel_includes,
  dependencies : libvaccel_deps,
  install : true)

libvaccel_private = declare_dependency(
  sources : vaccel_headers,
  include_directories : vaccel_includes,
  dependencies : libvaccel_deps,
  link_with: libvaccel)

pkg_mod = import('pkgconfig')
pkg_mod.generate(libvaccel,
  version : libvaccel_version,
  subdirs: ['.', 'vaccel'],
  name : 'vaccel',
  description : 'Hardware Acceleration for Serverless Computing')
