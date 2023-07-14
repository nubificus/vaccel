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
  'tf_model.c',
  'tf_inference.c',
  'tf_saved_model.c',
  'pynq_vector_add.c',
  'pynq_vector_add_generic.c',
  'pynq_parallel.c',
  'pynq_parallel_generic.c',
  'pynq_array_copy.c',
  'pynq_array_copy_generic.c',
  'torch_inference.cpp',
  'exec_helpers.c',
  'exec_helpers_nonser.c',
])

foreach e : examples_sources
  exe = executable(fs.stem(e),
    sources : e,
    c_args : vaccel_cargs,
    include_directories : vaccel_includes,
    dependencies : [libvaccel_private],
    install : true)
endforeach

mytestlib = library('mytestlib',
  files('mytestlib.c'),
  c_args : vaccel_cargs,
  include_directories : vaccel_includes,
  dependencies : [libvaccel_private],
  install : true)

foreach d : ['models', 'images', 'input']
  install_subdir(d, install_dir : 'share/vaccel')
endforeach
