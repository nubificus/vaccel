conf = configuration_data()
conf.set('VACCEL_VERSION', meson.project_version())

vaccel_h = configure_file(input : 'vaccel.h.in',
  output : 'vaccel.h',
  configuration : conf)

vaccel_headers = files([
  'arg.h',
  'config.h',
  'core.h',
  'error.h',
  'file.h',
  'id_pool.h',
  'list.h',
  'log.h',
  'plugin.h',
  'prof.h',
  'op.h',
  'resource.h',
  'session.h',
])

vaccel_sources = files([
  'arg.c',
  'config.c',
  'file.c',
  'id_pool.c',
  'log.c',
  'plugin.c',
  'prof.c',
  'resource.c',
  'session.c',
  'vaccel.c',
])

subdir('include')
subdir('ops')
subdir('utils')

vaccel_sources += [vaccel_headers, vaccel_h, vaccel_public_headers]
vaccel_includes = include_directories('.')
vaccel_public_includes = include_directories('include')

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
  include_directories : vaccel_public_includes,
  dependencies : libvaccel_deps,
  install : true)

libvaccel_dep = declare_dependency(
  sources : vaccel_public_headers,
  include_directories : vaccel_public_includes,
  dependencies : libvaccel_deps,
  link_with: libvaccel)

libvaccel_internal_dep = declare_dependency(
  sources : vaccel_headers,
  include_directories : [vaccel_includes, vaccel_public_includes],
  dependencies : libvaccel_deps,
  link_with: libvaccel)

pkg_mod = import('pkgconfig')
pkg_mod.generate(libvaccel,
  version : libvaccel_version,
  subdirs: '.',
  name : 'vaccel',
  description : 'Hardware Acceleration for Serverless Computing')
