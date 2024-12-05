python_sources = files([
  'helper.c',
])

libvaccel_python = library('vaccel-python',
  python_sources,
  version: libvaccel_version,
  c_args : vaccel_c_args,
  dependencies : [libvaccel_dep],
  install : true)

pkg_mod = import('pkgconfig')
pkg_mod.generate(libvaccel_python,
  version : libvaccel_version,
  subdirs: ['.', 'vaccel'],
  name : 'vaccel-python',
  description : 'Hardware Acceleration for Serverless Computing')
