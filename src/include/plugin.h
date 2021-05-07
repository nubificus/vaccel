#ifndef __VACCEL_PLUGIN_H__
#define __VACCEL_PLUGIN_H__

#include <stdint.h>

#include "session.h"
#include "resources.h"
#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

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

	/* In some cases, like in the context of VirtIO we need to offload
	 * session handling to the plugin itself */
	int (*sess_init)(struct vaccel_session *sess, uint32_t flags);
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
