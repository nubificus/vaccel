// SPDX-License-Identifier: Apache-2.0

#define _POSIX_C_SOURCE 200809L

#include "plugin.h"
#include "vaccel.h"
#include <assert.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct {
	/* true if the plugins component is initialized */
	bool initialized;

	/* list of registered plugins */
	vaccel_list_t registered;

	/* number of registered plugins */
	size_t nr_registered;

	/* virtio plugin (if available) */
	struct vaccel_plugin *virtio;

	/* array of available implementations for every supported
	 * function */
	vaccel_list_t ops[VACCEL_OP_MAX];
} plugins = { .initialized = false };

static int plugin_check_info(const struct vaccel_plugin_info *pinfo)
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

int plugin_parse_version(int *major, int *minor1, int *minor2, char **extra,
			 const char *str)
{
	if (!str)
		return VACCEL_EINVAL;

	char *tmp_str = strdup(str);
	if (!tmp_str)
		return VACCEL_ENOMEM;

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

static int plugin_check_version(const struct vaccel_plugin_info *pinfo)
{
	int ret = VACCEL_OK;

	const struct vaccel_config *config = vaccel_config();
	if (config->version_ignore)
		return VACCEL_OK;

	if (!pinfo->vaccel_version) {
		vaccel_error("The plugin has no vaccel version set");
		return VACCEL_EINVAL;
	}
	int major;
	int minor1;
	int minor2;
	char *extra = NULL;
	if (plugin_parse_version(&major, &minor1, &minor2, &extra,
				 pinfo->vaccel_version)) {
		vaccel_error("Could not parse plugin's vaccel version");
		ret = VACCEL_EINVAL;
		goto free_extra;
	}

	int vmajor;
	int vminor1;
	int vminor2;
	char *vextra = NULL;
	if (plugin_parse_version(&vmajor, &vminor1, &vminor2, &vextra,
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
	    (extra && vextra && strcmp(extra, vextra) != 0)) {
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

	return ret;
}

static int is_virtio_plugin(const struct vaccel_plugin_info *pinfo)
{
	return pinfo->session_init && pinfo->session_release;
}

int plugin_register(struct vaccel_plugin *plugin)
{
	int ret;

	if (!plugin) {
		vaccel_fatal("BUG! Invalid plugin struct");
		return VACCEL_EINVAL;
	}

	if (!plugins.initialized) {
		vaccel_fatal("BUG! Plugin system not initialized");
		return VACCEL_EBACKEND;
	}

	if (list_entry_linked(&plugin->entry)) {
		vaccel_error("Plugin already registered");
		return VACCEL_EEXIST;
	}

	if (!list_empty(&plugin->ops))
		return VACCEL_EINVAL;

	ret = plugin_check_info(plugin->info);
	if (ret)
		return ret;

	ret = plugin_check_version(plugin->info);
	if (ret)
		return ret;

	const struct vaccel_plugin_info *info = plugin->info;

	list_add_tail(&plugins.registered, &plugin->entry);
	plugins.nr_registered++;

	vaccel_info("Registered plugin %s %s", info->name, info->version);

	if (is_virtio_plugin(info)) {
		vaccel_debug("%s is a VirtIO module", info->name);

		if (plugins.virtio)
			vaccel_debug(
				"A VirtIO registered is already registered");
		else {
			plugins.virtio = plugin;
		}
	}

	return VACCEL_OK;
}

int plugin_unregister(struct vaccel_plugin *plugin)
{
	if (!plugin)
		return VACCEL_EINVAL;

	if (!plugins.initialized)
		return VACCEL_EBACKEND;

	if (!plugin)
		return VACCEL_EINVAL;

	if (!list_entry_linked(&plugin->entry)) {
		assert(0 && "Trying to unregister unknown plugin");
		return VACCEL_ENOENT;
	}

	/* unregister plugin's functions */
	struct vaccel_op *op;
	struct vaccel_op *tmp;
	list_for_each_container_safe(op, tmp, &plugin->ops, struct vaccel_op,
				     plugin_entry)
	{
		list_unlink_entry(&op->func_entry);
		list_unlink_entry(&op->plugin_entry);
	}

	list_unlink_entry(&plugin->entry);
	plugins.nr_registered--;

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

int vaccel_plugin_register_op(struct vaccel_op *op)
{
	if (!op || !op->func) {
		vaccel_error("Invalid vaccel function");
		return VACCEL_EINVAL;
	}

	if (op->type >= VACCEL_OP_MAX) {
		vaccel_error("Unknown function type");
		return VACCEL_EINVAL;
	}

	struct vaccel_plugin *plugin = op->owner;
	if (!plugin) {
		vaccel_error("Unknown plugin");
		return VACCEL_EINVAL;
	}

	list_add_tail(&plugin->ops, &op->plugin_entry);
	list_add_tail(&plugins.ops[op->type], &op->func_entry);

	char op_name[VACCEL_ENUM_STR_MAX];
	vaccel_debug("Registered op %s from plugin %s",
		     vaccel_op_type_name(op->type, op_name,
					 VACCEL_ENUM_STR_MAX),
		     plugin->info->name);

	return VACCEL_OK;
}

int vaccel_plugin_register_ops(struct vaccel_op *ops, size_t nr_ops)
{
	for (size_t i = 0; i < nr_ops; ++i) {
		int ret = vaccel_plugin_register_op(&ops[i]);
		if (ret)
			return ret;
	}

	return VACCEL_OK;
}

void *plugin_get_op_func(vaccel_op_type_t op_type, unsigned int hint)
{
	unsigned int env_priority = hint & (~VACCEL_PLUGIN_REMOTE);
	struct vaccel_op *op = NULL;
	struct vaccel_op *opiter;
	struct vaccel_op *tmp;

	if (op_type >= VACCEL_OP_MAX) {
		vaccel_error("Trying to execute unknown function");
		return NULL;
	}

	if (list_empty(&plugins.ops[op_type])) {
		vaccel_warn("None of the loaded plugins implement %s",
			    vaccel_op_type_to_str(op_type));
		return NULL;
	}

	/* Check the list of plugins implementing our function type
	 * and compare with the bitmap hint we got from the upper
	 * layers. If we get a match, return this plugin operation
	 */

	if (VACCEL_PLUGIN_REMOTE & hint) {
		list_for_each_container_safe(opiter, tmp, &plugins.ops[op_type],
					     struct vaccel_op, func_entry)
		{
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
		list_for_each_container_safe(opiter, tmp, &plugins.ops[op_type],
					     struct vaccel_op, func_entry)
		{
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
		list_for_each_container_safe(opiter, tmp, &plugins.ops[op_type],
					     struct vaccel_op, func_entry)
		{
			if (!opiter->owner->info->is_virtio ||
			    plugins.nr_registered == 1) {
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

void vaccel_plugin_print_all_by_op_type(vaccel_op_type_t op_type)
{
	struct vaccel_op *opiter;
	struct vaccel_op *tmp_op;

	list_for_each_container_safe(opiter, tmp_op, &plugins.ops[op_type],
				     struct vaccel_op, func_entry)
	{
		const char *p_type_str =
			vaccel_plugin_type_to_str(opiter->owner->info->type);
		vaccel_debug("Found implementation of %s in %s plugin type: %s",
			     vaccel_op_type_to_str(opiter->type),
			     opiter->owner->info->name, p_type_str);
	}
}

size_t plugin_nr_registered()
{
	return plugins.nr_registered;
}

struct vaccel_plugin *plugin_virtio()
{
	return plugins.virtio;
}

int vaccel_plugin_load(const char *lib)
{
	int ret;

	if (lib == NULL)
		return VACCEL_EINVAL;

	void *dl = dlopen(lib, RTLD_LAZY);
	if (!dl) {
		vaccel_error("Could not dlopen plugin %s: %s", lib, dlerror());
		return VACCEL_ENOENT;
	}

	struct vaccel_plugin **plugin =
		(struct vaccel_plugin **)dlsym(dl, "vaccel_plugin");
	if (!plugin) {
		vaccel_error("%s is not a vaccel plugin", lib);
		ret = VACCEL_ELIBBAD;
		goto close_dl;
	}

	ret = plugin_register(*plugin);
	if (ret != VACCEL_OK) {
		vaccel_error("Could not register plugin %s", lib);
		goto close_dl;
	}

	/* Initialize the plugin */
	ret = (*plugin)->info->init();
	if (ret != VACCEL_OK) {
		vaccel_error("Could not initialize plugin %s",
			     (*plugin)->info->name);
		goto unregister;
	}

	/* Keep dl handle so we can later close the library */
	(*plugin)->dl_handle = dl;

	vaccel_debug("Loaded plugin %s from %s", (*plugin)->info->name, lib);

	return VACCEL_OK;

unregister:
	plugin_unregister(*plugin);
close_dl:
	dlclose(dl);
	return ret;
}

int vaccel_plugin_parse_and_load(const char *lib_str)
{
	int ret = VACCEL_OK;

	if (lib_str == NULL)
		return VACCEL_EINVAL;

	char *lib_str_temp = strdup(lib_str);
	if (!lib_str_temp)
		return VACCEL_ENOMEM;

	char *lib = strtok(lib_str_temp, ":");
	while (lib) {
		ret = vaccel_plugin_load(lib);
		if (ret != VACCEL_OK)
			break;

		lib = strtok(NULL, ":");
	}

	free(lib_str_temp);
	return ret;
}

int plugins_bootstrap()
{
	list_init(&plugins.registered);

	for (size_t i = 0; i < VACCEL_OP_MAX; ++i)
		list_init(&plugins.ops[i]);

	plugins.initialized = true;

	return VACCEL_OK;
}

int plugins_cleanup()
{
	if (!plugins.initialized)
		return VACCEL_OK;

	vaccel_debug("Cleaning up plugins");

	struct vaccel_plugin *plugin;
	struct vaccel_plugin *tmp;
	list_for_each_container_safe(plugin, tmp, &plugins.registered,
				     struct vaccel_plugin, entry)
	{
		/* Unregister plugin from runtime */
		int ret = plugin_unregister(plugin);
		if (ret != VACCEL_OK) {
			vaccel_warn("Failed to remove plugin, ret: %d", ret);
			continue;
		}
	}

	if (plugins.virtio)
		plugins.virtio = NULL;

	plugins.initialized = false;

	return VACCEL_OK;
}
