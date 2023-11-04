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

#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <dlfcn.h>

#include "plugin.h"
#include "error.h"
#include "list.h"
#include "ops/vaccel_ops.h"
#include "log.h"

static struct {
	/* true if sub-system is initialized */
	bool initialized;

	/* list holding plugin descriptors */
	list_t plugins;

	/* virtio plugin */
	struct vaccel_plugin *virtio;

	/* array of available implementations for every supported
	 * function
	 */
	list_t ops[VACCEL_FUNCTIONS_NR];
} plugin_state = {0};

static int check_plugin_info(const struct vaccel_plugin_info *pinfo)
{
	if (!pinfo->name) {
		vaccel_error("Plugin needs to have a name");
		return VACCEL_EINVAL;
	}

	if (!pinfo->init) {
		vaccel_error("Plugin needs to expose an init function");
		return VACCEL_EINVAL;
	}

	if (!pinfo->fini) {
		vaccel_error("Plugin needs to expose a fini function");
		return VACCEL_EINVAL;
	}

	return VACCEL_OK;
}

static int is_virtio_plugin(const struct vaccel_plugin_info *pinfo)
{
	return pinfo->sess_init && pinfo->sess_free;
}

int register_plugin(struct vaccel_plugin *plugin)
{
	if (!plugin) {
		vaccel_fatal("BUG! Invalid plugin struct");
		return VACCEL_EINVAL;
	}

	if (!plugin_state.initialized) {
		vaccel_fatal("BUG! Plugin system not initialized");
		return VACCEL_EBACKEND;
	}

	if (entry_linked(&plugin->entry)) {
		vaccel_error("Plugin already registered");
		return VACCEL_EEXISTS;
	}

	if (!list_empty(&plugin->ops))
		return VACCEL_EINVAL;

	if (check_plugin_info(plugin->info))
		return VACCEL_EINVAL;

	const struct vaccel_plugin_info *info = plugin->info;

	list_add_tail(&plugin_state.plugins, &plugin->entry);

	vaccel_info("Registered plugin %s %s", info->name, info->version);

	if (is_virtio_plugin(info)) {
		vaccel_debug("%s is a VirtIO module", info->name);

		if (plugin_state.virtio)
			vaccel_debug("A VirtIO registered is already registered");
		else {
			plugin_state.virtio = plugin;
		}
	}

	return VACCEL_OK;
}

int unregister_plugin(struct vaccel_plugin *plugin)
{
	if (!plugin)
		return VACCEL_EINVAL;

	if (!plugin_state.initialized)
		return VACCEL_EBACKEND;

	if (!plugin)
		return VACCEL_EINVAL;

	if (!entry_linked(&plugin->entry)) {
		//assert(0 && "Trying to unregister unknown plugin");
		return VACCEL_ENOENT;
	}

	/* unregister plugin's functions */
	struct vaccel_op *op, *tmp;
	for_each_container_safe(op, tmp, &plugin->ops, struct vaccel_op, plugin_entry) {
		list_unlink_entry(&op->func_entry);
		list_unlink_entry(&op->plugin_entry);
	}

	list_unlink_entry(&plugin->entry);

	if (!plugin->info) {
		vaccel_error("Plugin is missing info entry");
		return VACCEL_EINVAL;
	}

	/* Clean-up plugin's resources */
	plugin->info->fini();

	vaccel_debug("Unregistered plugin %s", plugin->info->name);

	if (plugin->dl_handle)
		dlclose(plugin->dl_handle);

	return VACCEL_OK;
}

int register_plugin_function(struct vaccel_op *plugin_op)
{
	if (!plugin_op || !plugin_op->func) {
		vaccel_error("Invalid vaccel function");
		return VACCEL_EINVAL;
	}

	if (plugin_op->type >= VACCEL_FUNCTIONS_NR) {
		vaccel_error("Unknown function type");
		return VACCEL_EINVAL;
	}

	struct vaccel_plugin *plugin = plugin_op->owner;
	if (!plugin) {
		vaccel_error("Unknown plugin");
		return VACCEL_EINVAL;
	}

	list_add_tail(&plugin->ops, &plugin_op->plugin_entry);
	list_add_tail(&plugin_state.ops[plugin_op->type],
			&plugin_op->func_entry);

	vaccel_debug("Registered function %s from plugin %s",
			vaccel_op_type_str(plugin_op->type),
			plugin->info->name);

	return VACCEL_OK;
}

int register_plugin_functions(struct vaccel_op *plugin_ops, size_t nr_ops)
{
	for (size_t i = 0; i < nr_ops; ++i) {
		int ret = register_plugin_function(&plugin_ops[i]);
		if (ret)
			return ret;
	}

	return VACCEL_OK;
}

void *get_plugin_op(enum vaccel_op_type op_type, unsigned int hint)
{
	unsigned int env_priority = hint;
	struct vaccel_op *op = NULL;

	if (op_type >= VACCEL_FUNCTIONS_NR) {
		vaccel_error("Trying to execute unknown function");
		return NULL;
	}

	if (list_empty(&plugin_state.ops[op_type])) {
		vaccel_warn("None of the loaded plugins implement %s",
				vaccel_op_type_str(op_type));
		return NULL;
	}

	/* Check the list of plugins implementing our function type
	 * and compare with the bitmap hint we got from the upper
	 * layers. If we get a match, return this plugin operation
	 */
	if (env_priority) {
		struct vaccel_op *opiter, *tmp;
		for_each_container_safe(opiter, tmp, &plugin_state.ops[op_type],
				struct vaccel_op, func_entry) {
			if ((env_priority & opiter->owner->info->type) != 0) {
				op = opiter;
				vaccel_debug("Returning func from hint plugin %s ",
						opiter->owner->info->name);
				break;
			}
		}
	}

	/* If priority check fails, just return the first implementation we find */
	if (!op) {
		op = get_container(plugin_state.ops[op_type].next,
				struct vaccel_op, func_entry);
	}

	vaccel_debug("Found implementation in %s plugin", op->owner->info->name);

	return op->func;
}

int get_available_plugins(enum vaccel_op_type op_type) {

        struct vaccel_op *opiter, *tmp_op;

        for_each_container_safe(opiter, tmp_op, &plugin_state.ops[op_type],
			struct vaccel_op, func_entry) {
		vaccel_debug("Found implementation of %s in %s plugin type: %s",
				vaccel_op_type_str(opiter->type),
				opiter->owner->info->name,
				vaccel_plugin_type_str(opiter->owner->info->type));
        }
	return 0;
}

struct vaccel_plugin *get_virtio_plugin(void)
{
	return plugin_state.virtio;
}

int plugins_bootstrap()
{
	list_init(&plugin_state.plugins);

	for (size_t i = 0; i < VACCEL_FUNCTIONS_NR; ++i)
		list_init(&plugin_state.ops[i]);

	plugin_state.initialized = true;

	return VACCEL_OK;
}

int plugins_shutdown(void)
{
	if (!plugin_state.initialized)
		return VACCEL_OK;

	vaccel_debug("Cleaning up plugins");

	struct vaccel_plugin *plugin, *tmp;
	for_each_container_safe(plugin, tmp, &plugin_state.plugins, struct vaccel_plugin, entry) {
		/* Unregister plugin from runtime */
		int ret = unregister_plugin(plugin);
		if (ret != VACCEL_OK) {
			vaccel_error("Failed to remove plugin, ret: %d", ret);
			continue;
		}
	}

	if (plugin_state.virtio)
		plugin_state.virtio = NULL;

	plugin_state.initialized = false;

	return VACCEL_OK;
}
