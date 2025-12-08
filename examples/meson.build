examples_sources = files([
  'classify.c',
  'classify_generic.c',
  'depth.c',
  'depth_generic.c',
  'detect.c',
  'detect_generic.c',
  'exec.c',
  'exec_generic.c',
  'exec_serialized.c',
  'exec_with_resource.c',
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

examples_includes = include_directories('.')

examples = []
foreach e : examples_sources
  name = fs.stem(e)
  if name.contains('inference')
    sources = [e, examples_common_inference_sources,
      examples_common_inference_headers]
  elif name.contains('serialized')
    sources = [e, examples_common_mydata_sources,
      examples_common_mydata_headers]
  else
    sources = e
  endif
  examples += executable(name,
    sources,
    c_args : vaccel_c_args,
    cpp_args : vaccel_cpp_args,
    dependencies : [libvaccel_internal_dep, libm_dep],
    install : true)
endforeach

libmytestlib = library('mytestlib',
  [files('mytestlib.c'),
    examples_common_mydata_sources, examples_common_mydata_headers],
  c_args : vaccel_c_args,
  dependencies : [libvaccel_internal_dep, libm_dep],
  install : true)

foreach d : ['models', 'images', 'input', 'labels']
  install_subdir(d, install_dir : 'share/vaccel')
endforeach
