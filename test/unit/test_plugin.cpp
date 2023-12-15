#include <catch2/catch_test_macros.hpp>

extern "C" {
#include "error.h"
#include "list.h"
#include "plugin.h"
}

#include <dlfcn.h>
#include <stdbool.h>
#include <string.h>

// TODO: Investigate unregister plugin function and freeing memory

static const char *pname = "mock_plugin_test";

static int fini(void)
{
    return VACCEL_OK;
}
static int init(void)
{
    return VACCEL_OK;
}

static int no_op(){return 2;}

TEST_CASE("get_all_available_functions")
{
    int ret;
    vaccel_plugin plugin;
    vaccel_plugin_info pinfo;
    plugin.dl_handle = nullptr;
    plugin.info = &pinfo;
    list_init_entry(&plugin.entry);
    list_init_entry(&plugin.ops);
    
    plugin.info->name = pname;
    plugin.info->init = init;
    plugin.info->fini = fini;
    plugin.info->is_virtio = false;
    plugin.info->type = VACCEL_PLUGIN_GENERIC;

    vaccel_op operation;
    operation.type = VACCEL_NO_OP;
    operation.func = (void*)no_op;
    operation.owner = &plugin;
    list_init_entry(&operation.plugin_entry);
    list_init_entry(&operation.func_entry);

    plugins_bootstrap();

    ret = register_plugin(&plugin);
    REQUIRE(ret == VACCEL_OK);

    ret = register_plugin_function(&operation);
    REQUIRE(ret == VACCEL_OK);

    ret = get_available_plugins(VACCEL_NO_OP);
    REQUIRE(ret == VACCEL_OK);

    ret = unregister_plugin(&plugin);
    REQUIRE(ret == VACCEL_OK);

    // plugins_shutdown();
}