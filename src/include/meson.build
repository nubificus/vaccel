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
  'ops/tflite.h',
  'ops/torch.h',
  'ops/vaccel_ops.h',
  'session.h',
  'vaccel_args.h',
  'resource.h',
  'file.h',
  'vaccel_id.h',
  'vaccel_prof.h',
  'list.h',
  'misc.h',
  'plugin.h',
])

vaccel_public_headers += include_vaccel_h

install_headers(vaccel_public_headers, subdir : 'vaccel', preserve_path : true)
