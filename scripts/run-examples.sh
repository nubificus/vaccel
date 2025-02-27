#!/bin/sh
# SPDX-License-Identifier: Apache-2.0

set -e

SCRIPTS_DIR=$(cd -- "$(dirname -- "$0")" >/dev/null && pwd -P)
# shellcheck source=scripts/config/variables.env
. "${SCRIPTS_DIR}/config/variables.env"

PREFIX=${1:-"/usr/local"}
VALGRIND=$2
[ "${VALGRIND}" = "true" ] && WRAPPER_CMD=${SCRIPTS_VALGRIND_CMD:-'valgrind'}
if [ -n "${MESON_BUILD_ROOT}" ]; then
	LIB_DIR=${MESON_BUILD_ROOT}/src
	EXAMPLES_DIR=${MESON_BUILD_ROOT}/examples
	TESTLIB_DIR=${EXAMPLES_DIR}
	SHARE_DIR=${MESON_SOURCE_ROOT}/examples
	NOOP_DIR=${MESON_BUILD_ROOT}/plugins/noop
	MBENCH_DIR=${MESON_BUILD_ROOT}/plugins/mbench
	EXEC_DIR=${MESON_BUILD_ROOT}/plugins/exec
else
	MULTIARCH_TRIPLET=$(dpkg-architecture -qDEB_HOST_MULTIARCH)
	LIB_DIR=${PREFIX}/lib/${MULTIARCH_TRIPLET}
	EXAMPLES_DIR=${PREFIX}/bin
	TESTLIB_DIR=${LIB_DIR}
	SHARE_DIR=${PREFIX}/share/vaccel
	NOOP_DIR=${LIB_DIR}
	MBENCH_DIR=${LIB_DIR}
	EXEC_DIR=${LIB_DIR}
fi

[ -z "${TERM}" ] && export TERM="linux"

export LD_LIBRARY_PATH="${LIB_DIR}"
export VACCEL_LOG_LEVEL=4
export VACCEL_PLUGINS="${NOOP_DIR}/libvaccel-noop.so"

printf "\n$(tput setaf 2)%s$(tput sgr0)\n" \
	"Run examples with plugin '${VACCEL_PLUGINS}'"
set -x
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}/noop"
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}/classify" \
	"${SHARE_DIR}/images/example.jpg" 1
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}/classify" \
	"${SHARE_DIR}/images/example.jpg" 1 \
	"${SHARE_DIR}/models/torch/cnn_trace.pt"
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}/classify_generic" \
	"${SHARE_DIR}/images/example.jpg" 1
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}/classify_generic" \
	"${SHARE_DIR}/images/example.jpg" 1 \
	"${SHARE_DIR}/models/torch/cnn_trace.pt"
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}/depth" \
	"${SHARE_DIR}/images/example.jpg" 1
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}/depth" \
	"${SHARE_DIR}/images/example.jpg" 1 \
	"${SHARE_DIR}/models/torch/cnn_trace.pt"
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}/depth_generic" \
	"${SHARE_DIR}/images/example.jpg" 1
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}/depth_generic" \
	"${SHARE_DIR}/images/example.jpg" 1 \
	"${SHARE_DIR}/models/torch/cnn_trace.pt"
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}/detect" \
	"${SHARE_DIR}/images/example.jpg" 1
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}/detect" \
	"${SHARE_DIR}/images/example.jpg" 1 \
	"${SHARE_DIR}/models/torch/cnn_trace.pt"
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}/detect_generic" \
	"${SHARE_DIR}/images/example.jpg" 1
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}/detect_generic" \
	"${SHARE_DIR}/images/example.jpg" 1 \
	"${SHARE_DIR}/models/torch/cnn_trace.pt"
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}/pose" \
	"${SHARE_DIR}/images/example.jpg" 1
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}/pose" \
	"${SHARE_DIR}/images/example.jpg" 1 \
	"${SHARE_DIR}/models/torch/cnn_trace.pt"
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}/pose_generic" \
	"${SHARE_DIR}/images/example.jpg" 1
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}/pose_generic" \
	"${SHARE_DIR}/images/example.jpg" 1 \
	"${SHARE_DIR}/models/torch/cnn_trace.pt"
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}/segment" \
	"${SHARE_DIR}/images/example.jpg" 1
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}/segment" \
	"${SHARE_DIR}/images/example.jpg" 1 \
	"${SHARE_DIR}/models/torch/cnn_trace.pt"
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}/segment_generic" \
	"${SHARE_DIR}/images/example.jpg" 1
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}/segment_generic" \
	"${SHARE_DIR}/images/example.jpg" 1 \
	"${SHARE_DIR}/models/torch/cnn_trace.pt"
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}/sgemm"
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}/sgemm_generic"
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}/minmax" 2048 \
	"${SHARE_DIR}/input/input_2048.csv" 5 100
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}/minmax_generic" 2048 \
	"${SHARE_DIR}/input/input_2048.csv" 5 100
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}/pynq_array_copy"
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}/pynq_array_copy_generic"
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}/pynq_parallel"
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}/pynq_parallel_generic"
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}/pynq_vector_add"
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}/pynq_vector_add_generic"
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}/tf_inference" \
	"${SHARE_DIR}/models/tf/lstm2"
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}/tflite_inference" \
	"${SHARE_DIR}/models/tf/lstm2.tflite"
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}/torch_inference" \
	"${SHARE_DIR}/images/example.jpg" \
	'https://s3.nbfc.io/torch/mobilenet.pt' \
	"${SHARE_DIR}/labels/imagenet.txt"
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}/mbench" 1 \
	"${SHARE_DIR}/images/example.jpg"
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}/exec" \
	"${TESTLIB_DIR}/libmytestlib.so" 1
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}/exec_generic" \
	"${TESTLIB_DIR}/libmytestlib.so" 1
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}/exec_helpers" \
	"${TESTLIB_DIR}/libmytestlib.so" 1
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}/exec_helpers_nonser" \
	"${TESTLIB_DIR}/libmytestlib.so" 1
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}/exec_with_res" \
	"${TESTLIB_DIR}/libmytestlib.so" 1
set +x

export VACCEL_PLUGINS="${MBENCH_DIR}/libvaccel-mbench.so"
printf "\n$(tput setaf 2)%s$(tput sgr0)\n" \
	"Run examples with plugin '${VACCEL_PLUGINS}'"
set -x
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}/mbench 1" \
	"${SHARE_DIR}/images/example.jpg"
set +x

export VACCEL_PLUGINS="${EXEC_DIR}/libvaccel-exec.so"
export VACCEL_EXEC_DLCLOSE=1
printf "\n$(tput setaf 2)%s$(tput sgr0)\n" \
	"Run examples with plugin '${VACCEL_PLUGINS}'"
set -x
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}/exec" \
	"${TESTLIB_DIR}/libmytestlib.so" 1
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}/exec_generic" \
	"${TESTLIB_DIR}/libmytestlib.so" 1
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}/exec_helpers" \
	"${TESTLIB_DIR}/libmytestlib.so" 1
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}/exec_helpers_nonser" \
	"${TESTLIB_DIR}/libmytestlib.so" 1
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}/exec_with_res" \
	"${TESTLIB_DIR}/libmytestlib.so" 1
