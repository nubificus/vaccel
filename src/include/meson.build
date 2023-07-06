include_vaccel_h = configure_file(input : 'vaccel.h.in',
  output : 'vaccel.h',
  configuration : conf)

vaccel_public_headers = files([
  'error.h',
  'log.h',
  'ops/blas.h',
  'ops/exec.h',
  'ops/fpga.h',
  'ops/genop.h',
  'ops/image.h',
  'ops/minmax.h',
  'ops/noop.h',
  'ops/opencv.h',
  'ops/tf.h',
  'ops/torch.h',
  'ops/vaccel_ops.h',
  'resources/shared_object.h',
  'resources/tf_saved_model.h',
  'resources/torch_saved_model.h',
  'session.h',
  'vaccel_file.h',
  'vaccel_prof.h',
  'list.h',
  'misc.h',
  'plugin.h',
  'resources.h',
  'tf_model.h',
  'vaccel_id.h',
  'vaccel_args.h',
])

vaccel_public_headers += include_vaccel_h

install_headers(vaccel_public_headers, subdir : 'vaccel', preserve_path : true)
