// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "list.h"
#include "utils/enum.h"
#include "utils/str.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Define vaccel_op_type_t, vaccel_op_type_to_str() and
 * vaccel_op_type_to_base_str() */
#define _ENUM_PREFIX VACCEL_OP
#define VACCEL_OP_TYPE_ENUM_LIST(VACCEL_ENUM_ITEM)          \
	VACCEL_ENUM_ITEM(NOOP, 0, _ENUM_PREFIX)             \
	VACCEL_ENUM_ITEM(EXEC, _ENUM_PREFIX)                \
	VACCEL_ENUM_ITEM(EXEC_WITH_RESOURCE, _ENUM_PREFIX)  \
	VACCEL_ENUM_ITEM(IMAGE_CLASSIFY, _ENUM_PREFIX)      \
	VACCEL_ENUM_ITEM(IMAGE_DETECT, _ENUM_PREFIX)        \
	VACCEL_ENUM_ITEM(IMAGE_SEGMENT, _ENUM_PREFIX)       \
	VACCEL_ENUM_ITEM(IMAGE_POSE, _ENUM_PREFIX)          \
	VACCEL_ENUM_ITEM(IMAGE_DEPTH, _ENUM_PREFIX)         \
	VACCEL_ENUM_ITEM(TF_MODEL_LOAD, _ENUM_PREFIX)       \
	VACCEL_ENUM_ITEM(TF_MODEL_UNLOAD, _ENUM_PREFIX)     \
	VACCEL_ENUM_ITEM(TF_MODEL_RUN, _ENUM_PREFIX)        \
	VACCEL_ENUM_ITEM(TFLITE_MODEL_LOAD, _ENUM_PREFIX)   \
	VACCEL_ENUM_ITEM(TFLITE_MODEL_UNLOAD, _ENUM_PREFIX) \
	VACCEL_ENUM_ITEM(TFLITE_MODEL_RUN, _ENUM_PREFIX)    \
	VACCEL_ENUM_ITEM(TORCH_MODEL_LOAD, _ENUM_PREFIX)    \
	VACCEL_ENUM_ITEM(TORCH_MODEL_RUN, _ENUM_PREFIX)     \
	VACCEL_ENUM_ITEM(TORCH_SGEMM, _ENUM_PREFIX)         \
	VACCEL_ENUM_ITEM(BLAS_SGEMM, _ENUM_PREFIX)          \
	VACCEL_ENUM_ITEM(FPGA_ARRAYCOPY, _ENUM_PREFIX)      \
	VACCEL_ENUM_ITEM(FPGA_MMULT, _ENUM_PREFIX)          \
	VACCEL_ENUM_ITEM(FPGA_PARALLEL, _ENUM_PREFIX)       \
	VACCEL_ENUM_ITEM(FPGA_VECTORADD, _ENUM_PREFIX)      \
	VACCEL_ENUM_ITEM(MINMAX, _ENUM_PREFIX)              \
	VACCEL_ENUM_ITEM(OPENCV, _ENUM_PREFIX)

VACCEL_ENUM_DEF_WITH_STR_FUNCS(vaccel_op_type, _ENUM_PREFIX,
			       VACCEL_OP_TYPE_ENUM_LIST)
#undef _ENUM_PREFIX

static inline char *vaccel_op_type_to_lower(vaccel_op_type_t op_type,
					    char *lower, size_t size)
{
	vaccel_str_to_lower(vaccel_op_type_to_base_str(op_type), lower, size,
			    NULL);
	return lower;
}

struct vaccel_plugin;
struct vaccel_op {
	/* operation type */
	vaccel_op_type_t type;

	/* function implementing the operation */
	void *func;

	/* plugin to which this implementation belongs */
	struct vaccel_plugin *owner;
};

extern struct vaccel_plugin vaccel_this_plugin;
#define VACCEL_OP_INIT(name, type, func) \
	{ (type), (func), (&vaccel_this_plugin) }

#ifdef __cplusplus
}
#endif
