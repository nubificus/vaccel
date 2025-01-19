include_vaccel_h = configure_file(input : 'vaccel.h.in',
  output : 'vaccel.h',
  configuration : conf)

vaccel_public_headers = files([
  'vaccel/arg.h',
  'vaccel/config.h',
  'vaccel/error.h',
  'vaccel/file.h',
  'vaccel/id.h',
  'vaccel/list.h',
  'vaccel/log.h',
  'vaccel/op.h',
  'vaccel/ops/blas.h',
  'vaccel/ops/exec.h',
  'vaccel/ops/fpga.h',
  'vaccel/ops/genop.h',
  'vaccel/ops/image.h',
  'vaccel/ops/minmax.h',
  'vaccel/ops/noop.h',
  'vaccel/ops/opencv.h',
  'vaccel/ops/tf.h',
  'vaccel/ops/tflite.h',
  'vaccel/ops/torch.h',
  'vaccel/plugin.h',
  'vaccel/prof.h',
  'vaccel/resource.h',
  'vaccel/session.h',
  'vaccel/utils/enum.h',
  'vaccel/utils/path.h',
  'vaccel/utils/str.h',
])

vaccel_public_headers += include_vaccel_h

install_headers(vaccel_public_headers, preserve_path : true)
