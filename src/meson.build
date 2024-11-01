conf = configuration_data()
conf.set('VACCEL_VERSION', meson.project_version())

vaccel_h = configure_file(input : 'vaccel.h.in',
  output : 'vaccel.h',
  configuration : conf)

vaccel_headers = files([
  'error.h',
  'log.h',
  'misc.h',
  'plugin.h',
  'session.h',
  'list.h',
  'id_pool.h',
  'resource.h',
  'file.h',
  'vaccel_args.h',
])

vaccel_sources = files([
  'log.c',
  'misc.c',
  'session.c',
  'resource.c',
  'file.c',
  'vaccel.c',
  'id_pool.c',
  'plugin.c',
  'vaccel_args.c',
])

subdir('include')
subdir('ops')
subdir('profiling')
subdir('utils')

vaccel_sources += [vaccel_headers, vaccel_h, vaccel_public_headers]
vaccel_includes = include_directories('include')
vaccel_src_includes = include_directories('.')

threads_dep = dependency('threads')
libvaccel_deps = [
  threads_dep,
  dependency('dl'),
  libslog_dep,
  libcurl_dep,
]
libvaccel = library('vaccel',
  vaccel_sources,
  version: libvaccel_version,
  c_args : vaccel_c_args,
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
