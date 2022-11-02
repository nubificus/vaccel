#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <vaccel.h>
#include <plugin.h>
#include <assert.h>

__attribute__((constructor))
void load_vaccel(void)
{
	printf("Loading libvaccel\n");
	void *dl = dlopen("libvaccel.so", RTLD_LAZY | RTLD_GLOBAL);
	if (!dl) {
		fprintf(stderr, "Could not open libvaccel\n");
		exit(1);
	}

	char *pname = getenv("PYTHON_VACCEL_PLUGIN");
	printf("Loading plugin %s\n", pname);
	void *plugin = dlopen(pname, RTLD_NOW);
	if (!plugin) {
		fprintf(stderr, "Failed to load plugin: %s\n", dlerror());
		exit(1);
	}

	struct vaccel_plugin **p;

	int (*register_plugin)(struct vaccel_plugin *) =
		dlsym(dl, "register_plugin");
	assert(register_plugin);

	p = dlsym(plugin, "vaccel_plugin");
	if (p == NULL) {
		fprintf(stderr, "Cannot find vaccel_plugin symbol\n");
		return;
	}
	assert(p);

	int ret = register_plugin(*p);
	assert(!ret);
	if (ret) {
		fprintf(stderr, "Failed to register plugin\n");
		return;
	}
		
	ret = (*p)->info->init();
	assert(!ret);
	if (ret) {
		fprintf(stderr, "Failed to initialize plugin\n");
		return;
	}
}
