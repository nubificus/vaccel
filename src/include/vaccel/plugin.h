// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "list.h"
#include "op.h"
#include "resource.h"
#include "session.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	VACCEL_PLUGIN_CPU = 0x0001,
	VACCEL_PLUGIN_GPU = 0x0002,
	VACCEL_PLUGIN_FPGA = 0x0004,
	VACCEL_PLUGIN_SOFTWARE = 0x0008,
	VACCEL_PLUGIN_TENSORFLOW = 0x0010,
	VACCEL_PLUGIN_TORCH = 0x0020,
	VACCEL_PLUGIN_JETSON = 0x0040,
	VACCEL_PLUGIN_GENERIC = 0x0080,
	VACCEL_PLUGIN_DEBUG = 0x0100,
	VACCEL_REMOTE = 0x4000,
	VACCEL_PLUGIN_TYPE_MAX = 0x8000,
	VACCEL_PLUGIN_ALL = 0xffff,
} vaccel_plugin_t;

static const char *vaccel_plugin_type_name[] = {
	"CPU",	 "GPU",	   "FPGA",    "SOFTWARE", "TENSORFLOW",
	"TORCH", "JETSON", "GENERIC", "DEBUG",
};

/* Dummy type to str function for debug */
static inline char *vaccel_plugin_type_str(vaccel_plugin_t type)
{
	int i = 0;
	unsigned int tester = 1;
	char *p;
	char *plugin_type_str = (char *)malloc(100);
	if (!plugin_type_str)
		return NULL;

	p = plugin_type_str;
	for (i = 0; i < VACCEL_PLUGIN_TYPE_MAX >> 9; i++) {
		if (tester & type) {
			if (p != plugin_type_str) {
				sprintf(p, " ");
				p += strlen(" ");
			}
			sprintf(p, "%s", vaccel_plugin_type_name[i]);
			p += strlen(vaccel_plugin_type_name[i]);
		}
		tester <<= 1;
	}

	return plugin_type_str;
}

struct vaccel_plugin_info {
	/* name of the plugin */
	const char *name;

	/* human-readable version number */
	const char *version;

	/* human-readable vAccelRT version that the plugin has been built with */
	const char *vaccel_version;

	/* plugin initialization function */
	int (*init)(void);

	/* plugin cleaning-up function */
	int (*fini)(void);

	/* true if this is a VirtIO plugin */
	bool is_virtio;

	/* plugin enum type */
	vaccel_plugin_t type;

	/* in some cases, like in the context of VirtIO we need to offload
	 * session handling to the plugin itself */
	int (*session_init)(struct vaccel_session *sess, uint32_t flags);
	int (*session_update)(struct vaccel_session *sess, uint32_t flags);
	int (*session_release)(struct vaccel_session *sess);
	int (*resource_register)(struct vaccel_resource *res,
				 struct vaccel_session *sess);
	int (*resource_unregister)(struct vaccel_resource *res,
				   struct vaccel_session *sess);
};

struct vaccel_plugin {
	/* handle for dynamic library implementing plugin */
	void *dl_handle;

	/* entry for list of plugin */
	vaccel_list_entry_t entry;

	/* list of functions supported by this plugin */
	vaccel_list_t ops;

	/* plugin information */
	struct vaccel_plugin_info *info;
};

#define VACCEL_PLUGIN(...)                                                      \
	static struct vaccel_plugin_info _vaccel_plugin_info = { __VA_ARGS__ }; \
	__attribute__((visibility(                                              \
		"hidden"))) struct vaccel_plugin vaccel_this_plugin = {         \
		.dl_handle = NULL,                                              \
		.entry = LIST_ENTRY_INIT(vaccel_this_plugin.entry),             \
		.ops = LIST_ENTRY_INIT(vaccel_this_plugin.ops),                 \
		.info = &_vaccel_plugin_info                                    \
	};                                                                      \
	const struct vaccel_plugin *vaccel_plugin = &vaccel_this_plugin;

int vaccel_plugin_register_op(struct vaccel_op *op);
int vaccel_plugin_register_ops(struct vaccel_op *ops, size_t nr_ops);
int vaccel_plugin_print_all_by_op_type(vaccel_op_t op_type);
int vaccel_plugin_load(const char *lib);
int vaccel_plugin_parse_and_load(const char *lib_str);

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
