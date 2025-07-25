examples_sources = files([
  'classify.c',
  'classify_generic.c',
  'detect.c',
  'detect_generic.c',
  'segment.c',
  'segment_generic.c',
  'pose.c',
  'pose_generic.c',
  'depth.c',
  'depth_generic.c',
  'exec.c',
  'exec_with_res.c',
  'exec_generic.c',
  'noop.c',
  'sgemm.c',
  'sgemm_generic.c',
  'minmax.c',
  'minmax_generic.c',
  'tf_inference.c',
  'tflite_inference.c',
  'torch_inference.c',
  'pynq_vector_add.c',
  'pynq_vector_add_generic.c',
  'pynq_parallel.c',
  'pynq_parallel_generic.c',
  'pynq_array_copy.c',
  'pynq_array_copy_generic.c',
  'exec_helpers.c',
  'exec_helpers_nonser.c',
  'mbench.c',
  'local_and_virtio.c',
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
