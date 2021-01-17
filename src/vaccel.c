#include "plugin.h"
#include "session.h"
#include "log.h"
#include "vaccel.h"

#include <stdbool.h>
#include <string.h>
#include <dlfcn.h>
#include <stdio.h>

static int load_backend_plugin(const char *path)
{
	int ret;
	struct vaccel_plugin *plugin, **plugin_p;

	void *dl = dlopen(path, RTLD_LAZY);
	if (!dl) {
		vaccel_error("Could not dlopen plugin %s", path);
		return VACCEL_ENOENT;
	}

	plugin_p = dlsym(dl, "vaccel_plugin");
	if (!plugin_p) {
		vaccel_error("Not a vaccel plugin: %s", path);
		ret = VACCEL_ELIBBAD;
		goto close_dl;
	}

	plugin = *plugin_p;
	ret = register_plugin(plugin);
	if (ret != VACCEL_OK)
		goto close_dl;

	/* Initialize the plugin */
	ret = plugin->info->init();
	if (ret != VACCEL_OK)
		goto close_dl;

	/* Keep dl handle so we can later close the library */
	plugin->dl_handle = dl;

	vaccel_debug("Loaded plugin %s from %s", plugin->info->name, path);

	return VACCEL_OK;

close_dl:
	dlclose(dl);
	return ret;
}

static int load_backend_plugins(char *plugins)
{
	char *plugin = strtok(plugins, ";");
	while (plugin) {
		int ret = load_backend_plugin(plugin);
		if (ret != VACCEL_OK)
			return ret;

		plugin = strtok(NULL, ";");
	}

	return VACCEL_OK;
}

__attribute__((constructor))
static void vaccel_init(void)
{
	int ret = sessions_bootstrap();
	if (ret)
		return;

	/* Initialize logger */
	vaccel_log_init();

	vaccel_debug("Initializing vAccel");

	/* initialize the backends system */
	plugins_bootstrap();

	/* find backend implementations and set them up */
	char *plugins = getenv("VACCEL_BACKENDS");
	if (!plugins)
		return;

	load_backend_plugins(plugins);
}

__attribute__((destructor))
static void vaccel_fini(void)
{
	vaccel_debug("Shutting down vAccel");
	plugins_shutdown();
}
