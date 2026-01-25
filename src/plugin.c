// SPDX-License-Identifier: Apache-2.0

#define _POSIX_C_SOURCE 200809L

#include "plugin.h"
#include "vaccel.h"
#include <assert.h>
#include <dlfcn.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct {
	/* true if the plugins component is initialized */
	bool initialized;

	/* list of all the registered plugins */
	struct vaccel_list_entry all;

	/* counter for all registered plugins */
	size_t count;

	/* lock for list/counter */
	pthread_mutex_t lock;
} plugins = { .initialized = false };

int plugins_bootstrap()
{
	list_init(&plugins.all);
	plugins.count = 0;
	pthread_mutex_init(&plugins.lock, NULL);

	plugins.initialized = true;
	return VACCEL_OK;
}

int plugins_cleanup()
{
	if (!plugins.initialized)
		return VACCEL_OK;

	vaccel_debug("Cleaning up plugins");

	pthread_mutex_lock(&plugins.lock);

	struct vaccel_plugin *plugin;
	struct vaccel_plugin *tmp;
	plugin_for_each_safe(plugin, tmp, &plugins.all)
	{
		pthread_mutex_unlock(&plugins.lock);
		int ret = plugin_unregister(plugin);
		if (ret != VACCEL_OK)
			vaccel_warn("Failed to remove plugin, ret: %d", ret);
		pthread_mutex_lock(&plugins.lock);
	}

	pthread_mutex_unlock(&plugins.lock);

	pthread_mutex_destroy(&plugins.lock);
	plugins.initialized = false;

	return VACCEL_OK;
}

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
	int mj;
	int mn1;
	int mn2;
	char *ex;
	if (str[0] == 'v')
		// for backwards compatibility with vX.Y.Z version format
		ret = sscanf(tmp_str, "%*c%d.%d.%d%ms", &mj, &mn1, &mn2, &ex);
	else
		// parse X.Y.Z(-extra) version format
		ret = sscanf(tmp_str, "%d.%d.%d%ms", &mj, &mn1, &mn2, &ex);
	if (ret < 3) {
		free(tmp_str);
		return VACCEL_EINVAL;
	}

	*major = mj;
	*minor1 = mn1;
	*minor2 = mn2;
	*extra = ex;

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
	    (extra && vextra && strcmp(extra, vextra) != 0) ||
	    (extra && !vextra) || (!extra && vextra)) {
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

int plugin_register(struct vaccel_plugin *plugin)
{
	int ret;

	if (!plugin)
		return VACCEL_EINVAL;

	if (!plugins.initialized)
		return VACCEL_EBACKEND;

	if (list_entry_linked(&plugin->entry)) {
		vaccel_error("Plugin already registered");
		return VACCEL_EEXIST;
	}

	ret = plugin_check_info(plugin->info);
	if (ret)
		return ret;

	ret = plugin_check_version(plugin->info);
	if (ret)
		return ret;

	const struct vaccel_plugin_info *info = plugin->info;

	pthread_mutex_lock(&plugins.lock);
	list_add_tail(&plugins.all, &plugin->entry);
	plugins.count++;
	pthread_mutex_unlock(&plugins.lock);

	vaccel_info("Registered plugin %s %s", info->name, info->version);

	if (info->is_virtio)
		vaccel_debug("%s is a VirtIO module", info->name);

	return VACCEL_OK;
}

int plugin_unregister(struct vaccel_plugin *plugin)
{
	if (!plugin)
		return VACCEL_EINVAL;

	if (!plugins.initialized)
		return VACCEL_EBACKEND;

	if (!list_entry_linked(&plugin->entry)) {
		vaccel_error("Trying to unregister unknown plugin");
		return VACCEL_ENOENT;
	}

	if (!plugin->info) {
		vaccel_error("Plugin is missing info entry");
		return VACCEL_EINVAL;
	}

	pthread_mutex_lock(&plugins.lock);
	list_unlink_entry(&plugin->entry);
	plugins.count--;
	pthread_mutex_unlock(&plugins.lock);

	/* Clean-up plugin's resources */
	plugin->info->fini();

	vaccel_debug("Unregistered plugin %s", plugin->info->name);

	if (plugin->dl_handle)
		dlclose(plugin->dl_handle);

	return VACCEL_OK;
}

int vaccel_plugin_register_ops(struct vaccel_op *ops, size_t nr_ops)
{
	if (!ops || !nr_ops) {
		vaccel_error("No operations defined");
		return VACCEL_EINVAL;
	}

	if (nr_ops > VACCEL_OP_MAX) {
		vaccel_error("Invalid number of plugin operations");
		return VACCEL_EINVAL;
	}

	struct vaccel_plugin *plugin = ops[0].owner;
	for (size_t i = 0; i < nr_ops; ++i) {
		plugin->ops[ops[i].type] = &ops[i];

		char op_name[VACCEL_ENUM_STR_MAX];
		vaccel_debug("Registered op %s from plugin %s",
			     vaccel_op_type_name(ops[i].type, op_name,
						 VACCEL_ENUM_STR_MAX),
			     plugin->info->name);
	}

	return VACCEL_OK;
}

struct vaccel_plugin *plugin_find(unsigned int hint)
{
	struct vaccel_plugin *plugin = NULL;

	pthread_mutex_lock(&plugins.lock);

	if (list_empty(&plugins.all)) {
		pthread_mutex_unlock(&plugins.lock);
		vaccel_error("No plugins registered");
		return NULL;
	}

	/* If a transport plugin is requested, a plugin of this type must be
	 * registered */
	if (VACCEL_PLUGIN_REMOTE & hint) {
		plugin_for_each(plugin, &plugins.all)
		{
			if (plugin->info->is_virtio) {
				pthread_mutex_unlock(&plugins.lock);
				return plugin;
			}
		}

		pthread_mutex_unlock(&plugins.lock);
		vaccel_error("No VirtIO plugin registered");
		return NULL;
	}

	/* If an acceleration plugin is requested, try to find a plugin matching
	 * the specified hint */
	unsigned int env_priority = hint & (~VACCEL_PLUGIN_REMOTE);
	if (env_priority) {
		plugin_for_each(plugin, &plugins.all)
		{
			if ((env_priority & plugin->info->type) != 0) {
				pthread_mutex_unlock(&plugins.lock);
				return plugin;
			}
		}
	}

	/* If priority check fails, just return the first (local) implementation
	 * we find or any implementation if it's a single one */
	plugin_for_each(plugin, &plugins.all)
	{
		if (!plugin->info->is_virtio || plugins.count == 1) {
			pthread_mutex_unlock(&plugins.lock);
			return plugin;
		}
	}

	pthread_mutex_unlock(&plugins.lock);
	vaccel_error("No local plugin registered");

	return NULL;
}

struct vaccel_plugin *plugin_find_by_name(const char *name)
{
	if (!name)
		return NULL;

	pthread_mutex_lock(&plugins.lock);

	if (list_empty(&plugins.all)) {
		pthread_mutex_unlock(&plugins.lock);
		vaccel_error("No plugins registered");
		return NULL;
	}

	struct vaccel_plugin *plugin = NULL;
	plugin_for_each(plugin, &plugins.all)
	{
		if (strcmp(plugin->info->name, name) == 0) {
			pthread_mutex_unlock(&plugins.lock);
			return plugin;
		}
	}

	pthread_mutex_unlock(&plugins.lock);
	vaccel_error("Plugin '%s' is not registered", name);

	return NULL;
}

void *plugin_get_op_func(struct vaccel_plugin *plugin, vaccel_op_type_t op_type)
{
	if (!plugin)
		return NULL;

	if (op_type >= VACCEL_OP_MAX) {
		vaccel_error("Trying to execute unknown function");
		return NULL;
	}

	char op_name[VACCEL_ENUM_STR_MAX];
	if (!plugin->ops[op_type]) {
		vaccel_error(
			"Could not return func, plugin %s does not implement %s",
			plugin->info->name,
			vaccel_op_type_name(op_type, op_name,
					    VACCEL_ENUM_STR_MAX));
		return NULL;
	}

	vaccel_debug("Returning func for op %s from plugin %s",
		     vaccel_op_type_name(op_type, op_name, VACCEL_ENUM_STR_MAX),
		     plugin->info->name);
	return plugin->ops[op_type]->func;
}

size_t plugin_count()
{
	pthread_mutex_lock(&plugins.lock);
	size_t count = plugins.count;
	pthread_mutex_unlock(&plugins.lock);
	return count;
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
