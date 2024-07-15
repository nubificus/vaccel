#!/bin/sh
# SPDX-License-Identifier: Apache-2.0

PREFIX=${1:-"/usr/local"}
VALGRIND=$2
if [ -n "${MESON_BUILD_ROOT}" ]; then
	LIB_DIR=${MESON_BUILD_ROOT}/src
	EXAMPLES_DIR=${MESON_BUILD_ROOT}/examples
	TESTLIB_DIR=${EXAMPLES_DIR}
	SHARE_DIR=${MESON_SOURCE_ROOT}/examples
	NOOP_DIR=${MESON_BUILD_ROOT}/plugins/noop
	EXEC_DIR=${MESON_BUILD_ROOT}/plugins/exec
else
	MULTIARCH_TRIPLET=$(dpkg-architecture -qDEB_HOST_MULTIARCH)
	LIB_DIR=${PREFIX}/lib/${MULTIARCH_TRIPLET}
	EXAMPLES_DIR=${PREFIX}/bin
	TESTLIB_DIR=${LIB_DIR}
	SHARE_DIR=${PREFIX}/share/vaccel
	NOOP_DIR=${LIB_DIR}
	EXEC_DIR=${LIB_DIR}
fi

if [ "${VALGRIND}" = "true" ]; then
	WRAPPER_CMD="valgrind"
	WRAPPER_CMD="${WRAPPER_CMD} --leak-check=full --show-leak-kinds=all"
	WRAPPER_CMD="${WRAPPER_CMD} --track-origins=yes --errors-for-leak-kinds=all"
	WRAPPER_CMD="${WRAPPER_CMD} --max-stackframe=3145916 --error-exitcode=1"
fi

[ -z "${TERM}" ] && export TERM="linux"

export LD_LIBRARY_PATH="${LIB_DIR}"
export VACCEL_DEBUG_LEVEL=4
export VACCEL_BACKENDS="${NOOP_DIR}"/libvaccel-noop.so

printf "\n$(tput setaf 2)%s$(tput sgr0)\n" \
	"Run examples with plugin '${VACCEL_BACKENDS}'"
set -x
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}"/classify \
	"${SHARE_DIR}"/images/example.jpg 1
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}"/classify_generic \
	"${SHARE_DIR}"/images/example.jpg 1
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}"/detect \
	"${SHARE_DIR}"/images/example.jpg 1
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}"/detect_generic \
	"${SHARE_DIR}"/images/example.jpg 1
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}"/pose \
	"${SHARE_DIR}"/images/example.jpg 1
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}"/pose_generic \
	"${SHARE_DIR}"/images/example.jpg 1
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}"/segment \
	"${SHARE_DIR}"/images/example.jpg 1
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}"/segment_generic \
	"${SHARE_DIR}"/images/example.jpg 1
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}"/depth \
	"${SHARE_DIR}"/images/example.jpg 1
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}"/depth_generic \
	"${SHARE_DIR}"/images/example.jpg 1
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}"/sgemm
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}"/sgemm_generic
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}"/minmax 262144 \
	"${SHARE_DIR}"/input/input_262144.csv 5 100
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}"/minmax_generic 262144 \
	"${SHARE_DIR}"/input/input_262144.csv 5 100
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}"/pynq_vector_add
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}"/pynq_array_copy
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}"/pynq_parallel
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}"/single_model \
	"${SHARE_DIR}"/models/tf/frozen_graph.pb
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}"/tf_saved_model \
	"${SHARE_DIR}"/models/tf/lstm2
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}"/tf_inference \
	"${SHARE_DIR}"/models/tf/lstm2
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}"/tflite_inference \
	"${SHARE_DIR}"/models/tf/lstm2.tflite
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}"/torch_inference \
	"${SHARE_DIR}"/images/example.jpg \
	"${SHARE_DIR}"/models/torch/cnn_trace.pt
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}"/mbench 1 \
	"${SHARE_DIR}"/images/example.jpg
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}"/exec_with_res \
	"${TESTLIB_DIR}"/libmytestlib.so 1
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}"/exec_helpers \
	"${TESTLIB_DIR}"/libmytestlib.so 1
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}"/exec_helpers_nonser \
	"${TESTLIB_DIR}"/libmytestlib.so 1
set +x

export VACCEL_BACKENDS="${EXEC_DIR}"/libvaccel-exec.so
export VACCEL_EXEC_DLCLOSE=1
printf "\n$(tput setaf 2)%s$(tput sgr0)\n" \
	"Run examples with plugin '${VACCEL_BACKENDS}'"

set -x
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}"/exec_with_res \
	"${TESTLIB_DIR}"/libmytestlib.so 1
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}"/exec_helpers \
	"${TESTLIB_DIR}"/libmytestlib.so 1
eval "${WRAPPER_CMD}" "${EXAMPLES_DIR}"/exec_helpers_nonser \
	"${TESTLIB_DIR}"/libmytestlib.so 1
