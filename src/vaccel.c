#include "vaccel.h"
#include "backend.h"
#include "session.h"
#include "log.h"

#include <stdbool.h>
#include <string.h>
#include <dlfcn.h>
#include <stdio.h>

static int load_backend_plugin(const char *path)
{
	int ret;

	void *dl = dlopen(path, RTLD_LAZY);
	if (!dl) {
		vaccel_error("Could not dlopen plugin %s", path);
		return VACCEL_ENOENT;
	}

	int (*init)(struct vaccel_backend *) = dlsym(dl, "vaccel_backend_initialize");
	if (!init) {
		vaccel_error("Could not load init function for plugin %s: %s",
				path, dlerror());
		ret = VACCEL_ELIBBAD;
		goto close_dl;
	}

	int (*fini)(struct vaccel_backend *) = dlsym(dl, "vaccel_backend_finalize");
	if (!fini) {
		vaccel_error("Could not load fini function for plugin %s: %s",
				path, dlerror());
		ret = VACCEL_ELIBBAD;
		goto close_dl;
	}

	struct vaccel_backend *new = malloc(sizeof(*new));
	if (!new) {
		vaccel_error("Could not allocate memory");
		ret = VACCEL_ENOMEM;
		goto close_dl;
	}

	ret = init(new);
	if (ret != VACCEL_OK) {
		vaccel_error("Plugin initialization failed");
		goto free_backend;
	}

	vaccel_debug("Loaded plugin %s", path);

	/* setup the info needed to clean-up the backend later */
	new->dl = dl;
	new->fini = fini;

	return VACCEL_OK;

free_backend:
	free(new);

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
	backends_bootstrap();

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
	cleanup_backends();
}
