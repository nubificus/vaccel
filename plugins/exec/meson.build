exec_sources = files([
  'vaccel.c',
])

libvaccel_exec = shared_library('vaccel-exec',
  exec_sources,
  version: libvaccel_version,
  include_directories : include_directories('.'),
  c_args : plugins_cargs,
  dependencies : libvaccel_private,
  install : true)
