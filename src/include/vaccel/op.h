// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "list.h"
#include "utils/enum.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Define vaccel_op_type_t, vaccel_op_type_to_str() and
 * vaccel_op_type_to_base_str() */
#define _ENUM_PREFIX VACCEL_OP
#define VACCEL_OP_TYPE_ENUM_LIST(VACCEL_ENUM_ITEM)            \
	VACCEL_ENUM_ITEM(NOOP, 0, _ENUM_PREFIX)               \
	VACCEL_ENUM_ITEM(BLAS_SGEMM, _ENUM_PREFIX)            \
	VACCEL_ENUM_ITEM(IMAGE_CLASSIFY, _ENUM_PREFIX)        \
	VACCEL_ENUM_ITEM(IMAGE_DETECT, _ENUM_PREFIX)          \
	VACCEL_ENUM_ITEM(IMAGE_SEGMENT, _ENUM_PREFIX)         \
	VACCEL_ENUM_ITEM(IMAGE_POSE, _ENUM_PREFIX)            \
	VACCEL_ENUM_ITEM(IMAGE_DEPTH, _ENUM_PREFIX)           \
	VACCEL_ENUM_ITEM(EXEC, _ENUM_PREFIX)                  \
	VACCEL_ENUM_ITEM(TF_MODEL_NEW, _ENUM_PREFIX)          \
	VACCEL_ENUM_ITEM(TF_MODEL_DESTROY, _ENUM_PREFIX)      \
	VACCEL_ENUM_ITEM(TF_MODEL_REGISTER, _ENUM_PREFIX)     \
	VACCEL_ENUM_ITEM(TF_MODEL_UNREGISTER, _ENUM_PREFIX)   \
	VACCEL_ENUM_ITEM(TF_SESSION_LOAD, _ENUM_PREFIX)       \
	VACCEL_ENUM_ITEM(TF_SESSION_RUN, _ENUM_PREFIX)        \
	VACCEL_ENUM_ITEM(TF_SESSION_DELETE, _ENUM_PREFIX)     \
	VACCEL_ENUM_ITEM(MINMAX, _ENUM_PREFIX)                \
	VACCEL_ENUM_ITEM(FPGA_ARRAYCOPY, _ENUM_PREFIX)        \
	VACCEL_ENUM_ITEM(FPGA_MMULT, _ENUM_PREFIX)            \
	VACCEL_ENUM_ITEM(FPGA_PARALLEL, _ENUM_PREFIX)         \
	VACCEL_ENUM_ITEM(FPGA_VECTORADD, _ENUM_PREFIX)        \
	VACCEL_ENUM_ITEM(EXEC_WITH_RESOURCE, _ENUM_PREFIX)    \
	VACCEL_ENUM_ITEM(TORCH_JITLOAD_FORWARD, _ENUM_PREFIX) \
	VACCEL_ENUM_ITEM(TORCH_SGEMM, _ENUM_PREFIX)           \
	VACCEL_ENUM_ITEM(OPENCV, _ENUM_PREFIX)                \
	VACCEL_ENUM_ITEM(TFLITE_SESSION_LOAD, _ENUM_PREFIX)   \
	VACCEL_ENUM_ITEM(TFLITE_SESSION_RUN, _ENUM_PREFIX)    \
	VACCEL_ENUM_ITEM(TFLITE_SESSION_DELETE, _ENUM_PREFIX)

VACCEL_ENUM_DEF_WITH_STR_FUNCS(vaccel_op_type, _ENUM_PREFIX,
			       VACCEL_OP_TYPE_ENUM_LIST)
#undef _ENUM_PREFIX

struct vaccel_plugin;
struct vaccel_op {
	/* operation type */
	vaccel_op_type_t type;

	/* function implementing the operation */
	void *func;

	/* plugin to which this implementation belongs */
	struct vaccel_plugin *owner;

	/* entry for list of plugin functions */
	vaccel_list_entry_t plugin_entry;

	/* entry for global list of functions of this type */
	vaccel_list_entry_t func_entry;
};

extern struct vaccel_plugin vaccel_this_plugin;
#define VACCEL_OP_INIT(name, type, func)         \
	{ (type), (func), (&vaccel_this_plugin), \
	  LIST_ENTRY_INIT((name).plugin_entry),  \
	  LIST_ENTRY_INIT((name).func_entry) }

#ifdef __cplusplus
}
#endif
