// SPDX-License-Identifier: Apache-2.0

#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "plugin.h"
#include "error.h"
#include "list.h"
#include "log.h"
#include "ops/vaccel_ops.h"

static struct {
	/* true if sub-system is initialized */
	bool initialized;

	/* list holding plugin descriptors */
	list_t plugins;

	/* number of registered plugins */
	size_t nr_plugins;

	/* virtio plugin */
	struct vaccel_plugin *virtio;

	/* array of available implementations for every supported
	 * function
	 */
	list_t ops[VACCEL_FUNCTIONS_NR];
} plugin_state = { 0 };

static int check_plugin_info(const struct vaccel_plugin_info *pinfo)
{
	if (!pinfo->name) {
		vaccel_error("Plugin needs to have a name");
		return VACCEL_EINVAL;
	}

	if (!pinfo->version) {
		vaccel_error("Plugin needs to have a version");
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

int parse_plugin_version(int *major, int *minor1, int *minor2, char **extra,
			 const char *str)
{
	if (!str)
		return VACCEL_EINVAL;

	char *tmp_str = strdup(str);

	int ret;
	if (str[0] == 'v')
		// for backwards compatibility with vX.Y.Z version format
		ret = sscanf(tmp_str, "%*c%d.%d.%d%ms", major, minor1, minor2,
			     extra);
	else
		// parse X.Y.Z(-extra) version format
		ret = sscanf(tmp_str, "%d.%d.%d%ms", major, minor1, minor2,
			     extra);
	if (ret < 3) {
		free(tmp_str);
		return VACCEL_EINVAL;
	}

	free(tmp_str);
	return VACCEL_OK;
}

static int check_plugin_version(const struct vaccel_plugin_info *pinfo)
{
	int ret = VACCEL_OK;

	char *ignore = getenv("VACCEL_IGNORE_VERSION");
	if (ignore && (strcmp(ignore, "1") == 0 || strcmp(ignore, "true") == 0))
		return VACCEL_OK;

	if (!pinfo->vaccel_version) {
		vaccel_error("The plugin has no vaccel version set");
		return VACCEL_EINVAL;
	}
	int major;
	int minor1;
	int minor2;
	char *extra = NULL;
	if (parse_plugin_version(&major, &minor1, &minor2, &extra,
				 pinfo->vaccel_version)) {
		vaccel_error("Could not parse plugin's vaccel version");
		ret = VACCEL_EINVAL;
		goto free_extra;
	}

	int vmajor;
	int vminor1;
	int vminor2;
	char *vextra = NULL;
	if (parse_plugin_version(&vmajor, &vminor1, &vminor2, &vextra,
				 VACCEL_VERSION)) {
		vaccel_error("Could not parse vaccel version");
		ret = VACCEL_EINVAL;
		goto free_vextra;
	}

	if (major != vmajor) {
		vaccel_error(
			"Plugin is incompatible with current vAccel version (built w/ %s, used w/ %s)",
			pinfo->vaccel_version, VACCEL_VERSION);
		ret = VACCEL_EINVAL;
		goto free_vextra;
	}

	if (minor1 != vminor1 || minor2 != vminor2 ||
	    strcmp(extra, vextra) != 0) {
		vaccel_warn(
			"Plugin may be incompatible with current vAccel version (built w/ %s, used w/ %s)",
			pinfo->vaccel_version, VACCEL_VERSION);
	}

free_vextra:
	if (vextra)
		free(vextra);
free_extra:
	if (extra)
		free(extra);
	free(ignore);

	return ret;
}

static int is_virtio_plugin(const struct vaccel_plugin_info *pinfo)
{
	return pinfo->session_init && pinfo->session_release;
}

int register_plugin(struct vaccel_plugin *plugin)
{
	int ret;

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
		return VACCEL_EEXIST;
	}

	if (!list_empty(&plugin->ops))
		return VACCEL_EINVAL;

	ret = check_plugin_info(plugin->info);
	if (ret)
		return ret;

	ret = check_plugin_version(plugin->info);
	if (ret)
		return ret;

	const struct vaccel_plugin_info *info = plugin->info;

	list_add_tail(&plugin_state.plugins, &plugin->entry);
	plugin_state.nr_plugins++;

	vaccel_info("Registered plugin %s %s", info->name, info->version);

	if (is_virtio_plugin(info)) {
		vaccel_debug("%s is a VirtIO module", info->name);

		if (plugin_state.virtio)
			vaccel_debug(
				"A VirtIO registered is already registered");
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
		assert(0 && "Trying to unregister unknown plugin");
		return VACCEL_ENOENT;
	}

	/* unregister plugin's functions */
	struct vaccel_op *op;
	struct vaccel_op *tmp;
	for_each_container_safe(op, tmp, &plugin->ops, struct vaccel_op,
				plugin_entry) {
		list_unlink_entry(&op->func_entry);
		list_unlink_entry(&op->plugin_entry);
	}

	list_unlink_entry(&plugin->entry);
	plugin_state.nr_plugins--;

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
		     vaccel_op_type_str(plugin_op->type), plugin->info->name);

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
	unsigned int env_priority = hint & (~VACCEL_REMOTE);
	struct vaccel_op *op = NULL;
	struct vaccel_op *opiter;
	struct vaccel_op *tmp;

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

	if (VACCEL_REMOTE & hint) {
		for_each_container_safe(opiter, tmp, &plugin_state.ops[op_type],
					struct vaccel_op, func_entry) {
			if (opiter->owner->info->is_virtio) {
				op = opiter;
				vaccel_debug(
					"Returning func from hint plugin %s ",
					opiter->owner->info->name);
				goto out;
			}
		}
		vaccel_error(
			"Could not return func, no VirtIO plugin loaded yet");
		return NULL;
	}

	if (env_priority) {
		for_each_container_safe(opiter, tmp, &plugin_state.ops[op_type],
					struct vaccel_op, func_entry) {
			if ((env_priority & opiter->owner->info->type) != 0) {
				op = opiter;
				vaccel_debug(
					"Returning func from hint plugin %s ",
					opiter->owner->info->name);
				goto out;
			}
		}
	}

	// If priority check fails, just return the first (local) implementation we find
	// or any implementation if it's a single one
	if (!op) {
		for_each_container_safe(opiter, tmp, &plugin_state.ops[op_type],
					struct vaccel_op, func_entry) {
			if (!opiter->owner->info->is_virtio ||
			    plugin_state.nr_plugins == 1) {
				op = opiter;
				vaccel_debug(
					"Returning func from hint plugin %s ",
					opiter->owner->info->name);
				goto out;
			}
		}
		vaccel_error(
			"Could not return func, no local plugin loaded yet");
		return NULL;
	}

out:
	vaccel_debug("Found implementation in %s plugin",
		     op->owner->info->name);

	return op->func;
}

int get_available_plugins(enum vaccel_op_type op_type)
{
	struct vaccel_op *opiter;
	struct vaccel_op *tmp_op;

	for_each_container_safe(opiter, tmp_op, &plugin_state.ops[op_type],
				struct vaccel_op, func_entry) {
		char *p_type_str =
			vaccel_plugin_type_str(opiter->owner->info->type);
		if (!p_type_str)
			return VACCEL_ENOMEM;
		vaccel_debug("Found implementation of %s in %s plugin type: %s",
			     vaccel_op_type_str(opiter->type),
			     opiter->owner->info->name, p_type_str);
		free(p_type_str);
	}

	return VACCEL_OK;
}

size_t get_nr_plugins()
{
	return plugin_state.nr_plugins;
}

struct vaccel_plugin *get_virtio_plugin()
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

int plugins_shutdown()
{
	if (!plugin_state.initialized)
		return VACCEL_OK;

	vaccel_debug("Cleaning up plugins");

	struct vaccel_plugin *plugin;
	struct vaccel_plugin *tmp;
	for_each_container_safe(plugin, tmp, &plugin_state.plugins,
				struct vaccel_plugin, entry) {
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
