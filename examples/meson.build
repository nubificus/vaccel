examples_sources = files([
  'classify.c',
  'classify_generic.c',
  'depth.c',
  'depth_generic.c',
  'detect.c',
  'detect_generic.c',
  'exec.c',
  'exec_with_res.c',
  'exec_generic.c',
  'exec_helpers.c',
  'exec_helpers_nonser.c',
  'local_and_virtio.c',
  'mbench.c',
  'noop.c',
  'pose.c',
  'pose_generic.c',
  'segment.c',
  'segment_generic.c',
  'sgemm.c',
  'sgemm_generic.c',
  'tf_inference.c',
  'tflite_inference.c',
  'torch_inference.c',
])

subdir('common')

examples = []
foreach e : examples_sources
  name = fs.stem(e)
  if name.contains('inference')
    sources = [e, examples_common_sources, examples_common_headers]
  else
    sources = e
  endif
  examples += executable(name,
    sources,
    include_directories : include_directories('common'),
    c_args : vaccel_c_args,
    cpp_args : vaccel_cpp_args,
    dependencies : [libvaccel_internal_dep, libm_dep],
    install : true)
endforeach

mytestlib = library('mytestlib',
  files('mytestlib.c'),
  c_args : vaccel_c_args,
  dependencies : [libvaccel_internal_dep],
  install : true)

foreach d : ['models', 'images', 'input', 'labels']
  install_subdir(d, install_dir : 'share/vaccel')
endforeach
