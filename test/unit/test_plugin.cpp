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

// static const char *pname = "mock_plugin_test";

// // void dummy_function() {}

// static int fini(void)
// {
//     return VACCEL_OK;
// }
// static int init(void)
// {
//     return VACCEL_OK;
// }

// static int no_op(){return 2;}

TEST_CASE("get_all_available_functions")
{

    // vaccel_plugin no_op_plugin;
    // vaccel_plugin_info noop_pinfo;
    // int ret;

    // vaccel_op noop_operation;
    // noop_operation.type = VACCEL_NO_OP;
    // noop_operation.func = (void *)no_op;
    // noop_operation.owner = nullptr;

    // no_op_plugin.info = &noop_pinfo;
    // no_op_plugin.info->name = pname;
    // list_init_entry(&no_op_plugin.entry);
    // list_init_entry(&no_op_plugin.ops);
    // no_op_plugin.info->init = init;
    // no_op_plugin.info->fini = fini;

    plugins_bootstrap();

    // noop_operation.owner = &no_op_plugin;
    // ret = register_plugin(&no_op_plugin);
    // REQUIRE(ret ==  VACCEL_OK);

    // ret = register_plugin_function(&noop_operation);
    // REQUIRE(ret ==  VACCEL_OK);

    // ret = get_available_plugins(VACCEL_NO_OP);
    // REQUIRE(ret == VACCEL_OK);

    // ret = unregister_plugin(&no_op_plugin);
    // REQUIRE(ret == VACCEL_OK);

    plugins_shutdown();
}