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
  'torch_inference.cpp',
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
  'full_torch_classification.cpp',
])

examples = []
foreach e : examples_sources
  examples += executable(fs.stem(e),
    sources : e,
    c_args : vaccel_c_args,
    cpp_args : vaccel_cpp_args,
    dependencies : [libvaccel_internal_dep],
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
