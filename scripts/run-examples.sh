#!/bin/sh
# SPDX-License-Identifier: Apache-2.0

set -e

CONFIG_PKG=$1
CONFIG_USE_VALGRIND=$2
CONFIG_PKG_SCRIPTS_DIR=$(cd -- "$(dirname -- "$0")" >/dev/null && pwd -P)
CONFIG_DEF_CFG_DIR="${CONFIG_PKG_SCRIPTS_DIR}/common/config"

# shellcheck source=scripts/common/config/config-env.sh
. "${CONFIG_DEF_CFG_DIR}/config-env.sh"

if [ -n "${MESON_BUILD_ROOT}" ]; then
	EXAMPLES_DIR=${MESON_BUILD_ROOT}/examples
	TESTLIB_DIR=${EXAMPLES_DIR}
	SHARE_DIR=${MESON_SOURCE_ROOT}/examples
	LD_LIBRARY_PATH="${MESON_BUILD_ROOT}/plugins/noop:${LD_LIBRARY_PATH}"
	LD_LIBRARY_PATH="${MESON_BUILD_ROOT}/plugins/mbench:${LD_LIBRARY_PATH}"
	LD_LIBRARY_PATH="${MESON_BUILD_ROOT}/plugins/exec:${LD_LIBRARY_PATH}"
else
	EXAMPLES_DIR=${CONFIG_PKG_BIN_DIR}
	TESTLIB_DIR=${CONFIG_PKG_LIB_DIR}
	SHARE_DIR=${CONFIG_PKG_SHARE_DIR}
fi

export LD_LIBRARY_PATH

export VACCEL_PLUGINS=libvaccel-noop.so
printf "\n$(tput setaf 2)%s$(tput sgr0)\n" \
	"Running examples with plugin '${VACCEL_PLUGINS}'"
set -x
eval "${CONFIG_WRAPPER_CMD}" "${EXAMPLES_DIR}/noop"
eval "${CONFIG_WRAPPER_CMD}" "${EXAMPLES_DIR}/classify" \
	"${SHARE_DIR}/images/example.jpg" 1
eval "${CONFIG_WRAPPER_CMD}" "${EXAMPLES_DIR}/classify" \
	"${SHARE_DIR}/images/example.jpg" 1 \
	"${SHARE_DIR}/models/torch/cnn_trace.pt"
eval "${CONFIG_WRAPPER_CMD}" "${EXAMPLES_DIR}/classify_generic" \
	"${SHARE_DIR}/images/example.jpg" 1
eval "${CONFIG_WRAPPER_CMD}" "${EXAMPLES_DIR}/classify_generic" \
	"${SHARE_DIR}/images/example.jpg" 1 \
	"${SHARE_DIR}/models/torch/cnn_trace.pt"
eval "${CONFIG_WRAPPER_CMD}" "${EXAMPLES_DIR}/depth" \
	"${SHARE_DIR}/images/example.jpg" 1
eval "${CONFIG_WRAPPER_CMD}" "${EXAMPLES_DIR}/depth" \
	"${SHARE_DIR}/images/example.jpg" 1 \
	"${SHARE_DIR}/models/torch/cnn_trace.pt"
eval "${CONFIG_WRAPPER_CMD}" "${EXAMPLES_DIR}/depth_generic" \
	"${SHARE_DIR}/images/example.jpg" 1
eval "${CONFIG_WRAPPER_CMD}" "${EXAMPLES_DIR}/depth_generic" \
	"${SHARE_DIR}/images/example.jpg" 1 \
	"${SHARE_DIR}/models/torch/cnn_trace.pt"
eval "${CONFIG_WRAPPER_CMD}" "${EXAMPLES_DIR}/detect" \
	"${SHARE_DIR}/images/example.jpg" 1
eval "${CONFIG_WRAPPER_CMD}" "${EXAMPLES_DIR}/detect" \
	"${SHARE_DIR}/images/example.jpg" 1 \
	"${SHARE_DIR}/models/torch/cnn_trace.pt"
eval "${CONFIG_WRAPPER_CMD}" "${EXAMPLES_DIR}/detect_generic" \
	"${SHARE_DIR}/images/example.jpg" 1
eval "${CONFIG_WRAPPER_CMD}" "${EXAMPLES_DIR}/detect_generic" \
	"${SHARE_DIR}/images/example.jpg" 1 \
	"${SHARE_DIR}/models/torch/cnn_trace.pt"
eval "${CONFIG_WRAPPER_CMD}" "${EXAMPLES_DIR}/pose" \
	"${SHARE_DIR}/images/example.jpg" 1
eval "${CONFIG_WRAPPER_CMD}" "${EXAMPLES_DIR}/pose" \
	"${SHARE_DIR}/images/example.jpg" 1 \
	"${SHARE_DIR}/models/torch/cnn_trace.pt"
eval "${CONFIG_WRAPPER_CMD}" "${EXAMPLES_DIR}/pose_generic" \
	"${SHARE_DIR}/images/example.jpg" 1
eval "${CONFIG_WRAPPER_CMD}" "${EXAMPLES_DIR}/pose_generic" \
	"${SHARE_DIR}/images/example.jpg" 1 \
	"${SHARE_DIR}/models/torch/cnn_trace.pt"
eval "${CONFIG_WRAPPER_CMD}" "${EXAMPLES_DIR}/segment" \
	"${SHARE_DIR}/images/example.jpg" 1
eval "${CONFIG_WRAPPER_CMD}" "${EXAMPLES_DIR}/segment" \
	"${SHARE_DIR}/images/example.jpg" 1 \
	"${SHARE_DIR}/models/torch/cnn_trace.pt"
eval "${CONFIG_WRAPPER_CMD}" "${EXAMPLES_DIR}/segment_generic" \
	"${SHARE_DIR}/images/example.jpg" 1
eval "${CONFIG_WRAPPER_CMD}" "${EXAMPLES_DIR}/segment_generic" \
	"${SHARE_DIR}/images/example.jpg" 1 \
	"${SHARE_DIR}/models/torch/cnn_trace.pt"
eval "${CONFIG_WRAPPER_CMD}" "${EXAMPLES_DIR}/sgemm"
eval "${CONFIG_WRAPPER_CMD}" "${EXAMPLES_DIR}/sgemm_generic"
eval "${CONFIG_WRAPPER_CMD}" "${EXAMPLES_DIR}/minmax" 2048 \
	"${SHARE_DIR}/input/input_2048.csv" 5 100
eval "${CONFIG_WRAPPER_CMD}" "${EXAMPLES_DIR}/minmax_generic" 2048 \
	"${SHARE_DIR}/input/input_2048.csv" 5 100
eval "${CONFIG_WRAPPER_CMD}" "${EXAMPLES_DIR}/pynq_array_copy"
eval "${CONFIG_WRAPPER_CMD}" "${EXAMPLES_DIR}/pynq_array_copy_generic"
eval "${CONFIG_WRAPPER_CMD}" "${EXAMPLES_DIR}/pynq_parallel"
eval "${CONFIG_WRAPPER_CMD}" "${EXAMPLES_DIR}/pynq_parallel_generic"
eval "${CONFIG_WRAPPER_CMD}" "${EXAMPLES_DIR}/pynq_vector_add"
eval "${CONFIG_WRAPPER_CMD}" "${EXAMPLES_DIR}/pynq_vector_add_generic"
eval "${CONFIG_WRAPPER_CMD}" "${EXAMPLES_DIR}/tf_inference" \
	"${SHARE_DIR}/images/example.jpg" \
	'https://s3.nbfc.io/models/tf/resnet18-v2-7_saved_model.tar.xz' \
	"${SHARE_DIR}/labels/imagenet.txt"
eval "${CONFIG_WRAPPER_CMD}" "${EXAMPLES_DIR}/tflite_inference" \
	"${SHARE_DIR}/images/example.jpg" \
	'https://s3.nbfc.io/models/tf/resnet18-v2-7_float32.tflite' \
	"${SHARE_DIR}/labels/imagenet.txt"
eval "${CONFIG_WRAPPER_CMD}" "${EXAMPLES_DIR}/torch_inference" \
	"${SHARE_DIR}/images/example.jpg" \
	'https://s3.nbfc.io/torch/mobilenet.pt' \
	"${SHARE_DIR}/labels/imagenet.txt"
eval "${CONFIG_WRAPPER_CMD}" "${EXAMPLES_DIR}/mbench" 1 \
	"${SHARE_DIR}/images/example.jpg"
eval "${CONFIG_WRAPPER_CMD}" "${EXAMPLES_DIR}/exec" \
	"${TESTLIB_DIR}/libmytestlib.so" 1
eval "${CONFIG_WRAPPER_CMD}" "${EXAMPLES_DIR}/exec_generic" \
	"${TESTLIB_DIR}/libmytestlib.so" 1
eval "${CONFIG_WRAPPER_CMD}" "${EXAMPLES_DIR}/exec_helpers" \
	"${TESTLIB_DIR}/libmytestlib.so" 1
eval "${CONFIG_WRAPPER_CMD}" "${EXAMPLES_DIR}/exec_helpers_nonser" \
	"${TESTLIB_DIR}/libmytestlib.so" 1
eval "${CONFIG_WRAPPER_CMD}" "${EXAMPLES_DIR}/exec_with_res" \
	"${TESTLIB_DIR}/libmytestlib.so" 1
set +x

export VACCEL_PLUGINS=libvaccel-mbench.so
printf "\n$(tput setaf 2)%s$(tput sgr0)\n" \
	"Running examples with plugin '${VACCEL_PLUGINS}'"
set -x
eval "${CONFIG_WRAPPER_CMD}" "${EXAMPLES_DIR}/mbench 1" \
	"${SHARE_DIR}/images/example.jpg"
set +x

export VACCEL_PLUGINS=libvaccel-exec.so
export VACCEL_EXEC_DLCLOSE_ENABLED=1
printf "\n$(tput setaf 2)%s$(tput sgr0)\n" \
	"Running examples with plugin '${VACCEL_PLUGINS}'"
set -x
eval "${CONFIG_WRAPPER_CMD}" "${EXAMPLES_DIR}/exec" \
	"${TESTLIB_DIR}/libmytestlib.so" 1
eval "${CONFIG_WRAPPER_CMD}" "${EXAMPLES_DIR}/exec_generic" \
	"${TESTLIB_DIR}/libmytestlib.so" 1
eval "${CONFIG_WRAPPER_CMD}" "${EXAMPLES_DIR}/exec_helpers" \
	"${TESTLIB_DIR}/libmytestlib.so" 1
eval "${CONFIG_WRAPPER_CMD}" "${EXAMPLES_DIR}/exec_helpers_nonser" \
	"${TESTLIB_DIR}/libmytestlib.so" 1
eval "${CONFIG_WRAPPER_CMD}" "${EXAMPLES_DIR}/exec_with_res" \
	"${TESTLIB_DIR}/libmytestlib.so" 1
