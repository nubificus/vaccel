/*
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __VACCEL_PLUGIN_H__
#define __VACCEL_PLUGIN_H__

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "session.h"
#include "resources.h"
#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

enum vaccel_plugin_type {
	VACCEL_PLUGIN_CPU = 0x0001,
	VACCEL_PLUGIN_GPU = 0x0002,
	VACCEL_PLUGIN_FPGA = 0x0004,
	VACCEL_PLUGIN_SOFTWARE = 0x0008,
	VACCEL_PLUGIN_TENSORFLOW = 0x0010,
	VACCEL_PLUGIN_TORCH = 0x0020,
	VACCEL_PLUGIN_JETSON = 0x0040,
	VACCEL_PLUGIN_GENERIC = 0x0080,
	VACCEL_PLUGIN_DEBUG = 0x0100,
	VACCEL_PLUGIN_TYPE_MAX = 0x8000,
	VACCEL_PLUGIN_ALL = 0xffff,
};

static const char *vaccel_plugin_type_name[] = {
	"CPU",
	"GPU",
	"FPGA",
	"SOFTWARE",
	"TENSORFLOW",
	"TORCH",
	"JETSON",
	"GENERIC",
	"DEBUG",
};

/* Dummy type to str function for debug */
static inline const char *vaccel_plugin_type_str(enum vaccel_plugin_type type)
{
	int i = 0;
	char *p, *plugin_type_str = (char*)malloc(100);
	unsigned int tester = 1;
	p = plugin_type_str;
	for (i = 0; i < VACCEL_PLUGIN_TYPE_MAX>>9; i++) {
		if (tester & type) {
			if (p != plugin_type_str) {
				sprintf(p, " ");
				p+=strlen(" ");
			}
			sprintf(p, "%s", vaccel_plugin_type_name[i]);
			p+=strlen(vaccel_plugin_type_name[i]);
		}
		tester <<= 1;
	}

	return plugin_type_str;
}

struct vaccel_plugin_info {
	/* Name of the plugin */
	const char *name;

	/* human-readable version number */
	const char *version;

	/* Plugin initialization function */
	int (*init)(void);

	/* Plugin cleaning-up function */
	int (*fini)(void);

	/* True if this is a VirtIO plugin */
	bool is_virtio;

	/* Plugin enum type */
	enum vaccel_plugin_type type;

	/* In some cases, like in the context of VirtIO we need to offload
	 * session handling to the plugin itself */
	int (*sess_init)(struct vaccel_session *sess, uint32_t flags);
	int (*sess_update)(struct vaccel_session *sess, uint32_t flags);
	int (*sess_free)(struct vaccel_session *sess);
	int (*sess_register)(uint32_t sess_id, vaccel_id_t resource_id);
	int (*sess_unregister)(uint32_t sess_id, vaccel_id_t resource_id);
	int (*resource_new)(vaccel_resource_t, void *data, vaccel_id_t *id);
	int (*resource_destroy)(vaccel_id_t id);
};

struct vaccel_plugin {
	/* Handle for dynamic library implementing plugin */
	void *dl_handle;

	/* Entry for list of plugin */
	list_entry_t entry;

	/* List of functions supported by this plugin */
	list_t ops;

	/* Plugin information */
	struct vaccel_plugin_info *info;
};

extern struct vaccel_plugin _vaccel_this_plugin;
#define THIS_MODULE (&_vaccel_this_plugin)

#define VACCEL_MODULE(...)                                                     \
	static struct vaccel_plugin_info                                       \
			_vaccel_plugin_info = { __VA_ARGS__ };                 \
	__attribute__((visibility("hidden"))) struct vaccel_plugin             \
		_vaccel_this_plugin = {                                        \
			.dl_handle = NULL,                                     \
			.entry = LIST_ENTRY_INIT(_vaccel_this_plugin.entry),   \
			.ops = LIST_ENTRY_INIT(_vaccel_this_plugin.ops),       \
			.info = &_vaccel_plugin_info                           \
		};                                                             \
	const struct vaccel_plugin *vaccel_plugin = &_vaccel_this_plugin;

struct vaccel_op {
	/* operation type */
	uint8_t type;

	/* function implementing the operation */
	void *func;

	/* plugin to which this implementation belongs */
	struct vaccel_plugin *owner;

	/* Entry for list of plugin functions */
	list_entry_t plugin_entry;

	/* Entry for global list of functions of this type */
	list_entry_t func_entry;
};

#define VACCEL_OP_INIT(name, type, func)                                       \
	{                                                                      \
		(type),                                                        \
		(func),                                                        \
		THIS_MODULE,                                                   \
		LIST_ENTRY_INIT((name).plugin_entry),                          \
		LIST_ENTRY_INIT((name).func_entry)                             \
	}

int register_plugin(struct vaccel_plugin *plugin);
int unregister_plugin(struct vaccel_plugin *plugin);
int register_plugin_function(struct vaccel_op *plugin_op);
int register_plugin_functions(struct vaccel_op *plugin_ops, size_t nr_ops);

#ifdef __cplusplus
}
#endif

#endif /* __VACCEL_PLUGIN_H__ */
