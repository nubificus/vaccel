noop_sources = files([
  'vaccel.c',
])

libvaccel_noop = shared_library('vaccel-noop',
  noop_sources,
  version: libvaccel_version,
  include_directories : include_directories('.'),
  c_args : plugins_cargs,
  dependencies : libvaccel_private,
  install : true)
