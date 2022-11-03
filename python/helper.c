#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <plugin.h>
#include <vaccel.h>
#include <string.h>
#include <assert.h>

void *vacceldl = NULL;
char *pname = NULL;

/* Copied from vaccelrt/src/vaccel.c */
static int load_backend_plugin(const char *path)
{
	int ret;
	struct vaccel_plugin *plugin, **plugin_p;

	void *dl = dlopen(path, RTLD_LAZY);
	if (!dl) {
		printf("Could not dlopen plugin %s: %s", path, dlerror());
		return VACCEL_ENOENT;
	}

	plugin_p = dlsym(dl, "vaccel_plugin");
	if (!plugin_p) {
		printf("Not a vaccel plugin: %s", path);
		ret = VACCEL_ELIBBAD;
		goto close_dl;
	}

	plugin = *plugin_p;
	int (*register_plugin)(struct vaccel_plugin *) =
		dlsym(vacceldl, "register_plugin");
	assert(register_plugin);

	ret = register_plugin(plugin);
	if (ret != VACCEL_OK)
		goto close_dl;

	/* Initialize the plugin */
	ret = plugin->info->init();
	if (ret != VACCEL_OK)
		goto close_dl;

	/* Keep dl handle so we can later close the library */
	plugin->dl_handle = dl;

	printf("Loaded plugin %s from %s\n", plugin->info->name, path);

	return VACCEL_OK;

close_dl:
	dlclose(dl);
	return ret;
}

/* Copied from vaccelrt/src/vaccel.c */
int load_backend_plugins(char *plugins)
{
	char *plugin;

	char *plugins_tmp = strdup(plugins);
	if (!plugins_tmp)
		return VACCEL_ENOMEM;

	char *p = plugins_tmp;
	plugin = strtok(p, ":");
	while (plugin) {
		printf("Loading plugin: %s\n", plugin);
		int ret = load_backend_plugin(plugin);
		if (ret != VACCEL_OK)
			return ret;

		plugin = strtok(NULL, ":");
	}
	
	free(plugins_tmp);
	return VACCEL_OK;
}

__attribute__((constructor))
void load_vaccel(void)
{
	/* Get a copy of the plugin env variable */
	pname = strdup(getenv("VACCEL_BACKENDS"));

	/* Do not allow libvaccel.so to load the plugins
	   as we get unresolved symbols. See:
	   https://mail.python.org/pipermail/python-dev/2002-May/023923.html */
	unsetenv("VACCEL_BACKENDS");

	printf("Loading libvaccel\n");
	vacceldl = dlopen("libvaccel.so", RTLD_LAZY | RTLD_GLOBAL);
	if (!vacceldl) {
		fprintf(stderr, "Could not open libvaccel\n");
		exit(1);
	}

	/* Instead, load the plugins now */
	printf("Loading plugins\n");
	load_backend_plugins(pname);

}

__attribute__((destructor))
static void unload_vaccel(void)
{
        printf("Shutting down vAccel\n");
	/* This will unregister and free resources of the plugins */
	dlclose(vacceldl);
	if (pname)
		free(pname);
}
