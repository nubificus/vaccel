// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "list.h"
#include "op.h"
#include "resource.h"
#include "session.h"
#include "utils/enum.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Define vaccel_plugin_type_t, vaccel_plugin_type_to_str() and
 * vaccel_plugin_type_to_base_str() */
#define _ENUM_PREFIX VACCEL_PLUGIN
#define VACCEL_PLUGIN_TYPE_ENUM_LIST(VACCEL_ENUM_ITEM)     \
	VACCEL_ENUM_ITEM(CPU, 0x0001, _ENUM_PREFIX)        \
	VACCEL_ENUM_ITEM(GPU, 0x0002, _ENUM_PREFIX)        \
	VACCEL_ENUM_ITEM(FPGA, 0x0004, _ENUM_PREFIX)       \
	VACCEL_ENUM_ITEM(SOFTWARE, 0x0008, _ENUM_PREFIX)   \
	VACCEL_ENUM_ITEM(TENSORFLOW, 0x0010, _ENUM_PREFIX) \
	VACCEL_ENUM_ITEM(TORCH, 0x0020, _ENUM_PREFIX)      \
	VACCEL_ENUM_ITEM(JETSON, 0x0040, _ENUM_PREFIX)     \
	VACCEL_ENUM_ITEM(GENERIC, 0x0080, _ENUM_PREFIX)    \
	VACCEL_ENUM_ITEM(DEBUG, 0x0100, _ENUM_PREFIX)      \
	VACCEL_ENUM_ITEM(REMOTE, 0x4000, _ENUM_PREFIX)     \
	VACCEL_ENUM_ITEM(ALL, 0xffff, _ENUM_PREFIX)

VACCEL_ENUM_DEF_WITH_STR_FUNCS(vaccel_plugin_type, _ENUM_PREFIX,
			       VACCEL_PLUGIN_TYPE_ENUM_LIST)
#undef _ENUM_PREFIX

struct vaccel_plugin_info {
	/* name of the plugin */
	const char *name;

	/* human-readable version number */
	const char *version;

	/* human-readable vAccel version that the plugin has been built with */
	const char *vaccel_version;

	/* plugin initialization function */
	int (*init)(void);

	/* plugin cleaning-up function */
	int (*fini)(void);

	/* true if this is a VirtIO plugin */
	bool is_virtio;

	/* plugin enum type */
	vaccel_plugin_type_t type;

	/* in some cases, like in the context of VirtIO we need to offload
	 * session handling to the plugin itself */
	int (*session_init)(struct vaccel_session *sess, uint32_t flags);
	int (*session_release)(struct vaccel_session *sess);
	int (*session_update)(struct vaccel_session *sess, uint32_t flags);
	int (*resource_register)(struct vaccel_resource *res,
				 struct vaccel_session *sess);
	int (*resource_unregister)(struct vaccel_resource *res,
				   struct vaccel_session *sess);
	int (*resource_sync)(struct vaccel_resource *res,
			     struct vaccel_session *sess);
};

struct vaccel_plugin {
	/* handle for dynamic library implementing plugin */
	void *dl_handle;

	/* entry for list of plugin */
	struct vaccel_list_entry entry;

	/* array of operations supported by this plugin */
	struct vaccel_op *ops[VACCEL_OP_MAX];

	/* plugin information */
	struct vaccel_plugin_info *info;
};

/* Define plugin/plugin info.
 * Warning: This only works when used from C. */
#define VACCEL_PLUGIN(...)                                                      \
	static struct vaccel_plugin_info _vaccel_plugin_info = { __VA_ARGS__ }; \
	__attribute__((visibility(                                              \
		"hidden"))) struct vaccel_plugin vaccel_this_plugin = {         \
		.dl_handle = NULL,                                              \
		.entry = LIST_ENTRY_INIT(vaccel_this_plugin.entry),             \
		.ops = { NULL },                                                \
		.info = &_vaccel_plugin_info                                    \
	};                                                                      \
	const struct vaccel_plugin *vaccel_plugin = &vaccel_this_plugin;

/* Register operations from an array of operations */
int vaccel_plugin_register_ops(struct vaccel_op *ops, size_t nr_ops);

/* Load a plugin from a filename or path.
 * If a filename is provided, the compiler's library paths will be searched */
int vaccel_plugin_load(const char *lib);

/* Parse a `:` delimited string of plugins and load each plugin. */
int vaccel_plugin_parse_and_load(const char *lib_str);

/* Register single operation */
static inline int vaccel_plugin_register_op(struct vaccel_op *op)
{
	return vaccel_plugin_register_ops(op, 1);
}

/* Deprecated. To be removed. */
#define VACCEL_MODULE(...)                                                                    \
	_Pragma("GCC warning \"VACCEL_MODULE() is deprecated. Use VACCEL_PLUGIN() instead\"") \
		VACCEL_PLUGIN(__VA_ARGS__)

__attribute__((deprecated(
	"Use vaccel_plugin_register_op() instead"))) static inline int
register_plugin_function(struct vaccel_op *plugin_op)
{
	return vaccel_plugin_register_op(plugin_op);
}
__attribute__((deprecated(
	"Use vaccel_plugin_register_ops() instead"))) static inline int
register_plugin_functions(struct vaccel_op *plugin_ops, size_t nr_ops)
{
	return vaccel_plugin_register_ops(plugin_ops, nr_ops);
}

#ifdef __cplusplus
}
#endif
