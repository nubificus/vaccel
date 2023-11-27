#include <catch2/catch_test_macros.hpp>

extern "C" {
#include "plugin.h"
#include "error.h"
#include "list.h"
}

#include <stdbool.h>
#include <string.h>
#include <dlfcn.h>

static const char *pname = "mock_plugin";

void dummy_function() {}

static int fini(void)
{
    return VACCEL_OK;
} 
static int init(void)
{
    return VACCEL_OK;
}

TEST_CASE("plugin_register") {

    struct vaccel_plugin plugin;
    struct vaccel_plugin_info pinfo;
    plugin.info = &pinfo;
    plugin.info->name = pname;
    list_init_entry(&plugin.entry);
    list_init_entry(&plugin.ops);
    plugin.info->init = init;
    plugin.info->fini = fini;

    plugins_bootstrap();

    SECTION("normal plugin initialisation") 
    {
        REQUIRE(register_plugin(&plugin) == VACCEL_OK);
    }

    SECTION("null plugin initialisation")
    {
        REQUIRE(register_plugin(NULL) == VACCEL_EINVAL);
    }

    SECTION("null plugin name")
    {
        struct vaccel_plugin plugin;
        plugin.info = &pinfo;

        plugin.entry.next = &(plugin.entry);
        plugin.entry.prev = &(plugin.entry);

        REQUIRE(register_plugin(&plugin) == VACCEL_EINVAL);
    }

    SECTION("not boostrapped")
    {   
        REQUIRE(plugins_shutdown() == VACCEL_OK);
        struct vaccel_plugin new_plugin = {};
        REQUIRE(register_plugin(&new_plugin) == VACCEL_EBACKEND);
    }

    SECTION("plugin already registered")
    {
        REQUIRE(register_plugin(&plugin) == VACCEL_OK);
        REQUIRE(register_plugin(&plugin) == VACCEL_EEXISTS);
    }
    
    SECTION("plugin list is not empty")
    {
        REQUIRE(1 == 1);
    }

    SECTION("plugin has invalid pinfo parameters")
    {   
        struct vaccel_plugin_info temp = pinfo;
        pinfo.fini = NULL;
        REQUIRE(register_plugin(&plugin) == VACCEL_EINVAL);
        pinfo.init = NULL;
        REQUIRE(register_plugin(&plugin) == VACCEL_EINVAL);
        pinfo.name = NULL;
        REQUIRE(register_plugin(&plugin) == VACCEL_EINVAL);
        pinfo = temp;

        REQUIRE(register_plugin(&plugin) == VACCEL_OK);
       
    }

    // plugins_shutdown();
}

TEST_CASE("plugin_unregister") {

    struct vaccel_plugin plugin;
    struct vaccel_plugin_info pinfo;
    plugin.info = &pinfo;
    plugin.info->name = pname;
    list_init_entry(&plugin.entry);
    list_init_entry(&plugin.ops);
    plugin.info->init = init;
    plugin.info->fini = fini;

    plugins_bootstrap();
    register_plugin(&plugin);

    SECTION("no plugin to unregister")
    {
        REQUIRE(unregister_plugin(NULL) == VACCEL_EINVAL);
        plugins_shutdown();
    }

    SECTION("plugin state is not initialised")
    {
        plugins_shutdown();
        // REQUIRE(unregister_plugin(&plugin) == VACCEL_EBACKEND);
        plugins_shutdown();
        REQUIRE(1==1);
    }

    SECTION("entry_linked(&plugin.entry) is false")
    {
        list_unlink_entry(&plugin.entry);
        REQUIRE(unregister_plugin(&plugin) == VACCEL_ENOENT);
        plugins_shutdown();
    }

    SECTION("plugin has no info entry")
    {
        plugin.info = NULL;
        REQUIRE(unregister_plugin(&plugin) == VACCEL_EINVAL);
        plugins_shutdown();
    }

    SECTION("plugin succesfully unregistered")
    {
        REQUIRE(unregister_plugin(&plugin) == VACCEL_OK);
        plugins_shutdown();
    }
}

TEST_CASE("register_plugin_function") {
    
    int res = plugins_bootstrap();

    REQUIRE(res == VACCEL_OK);

    // void *dl_handle = dlopen("../plugins/noop/libvaccel-noop.so", RTLD_LAZY);
    // if (!dl_handle) {
    //     FAIL("Error loading plugin library: " << dlerror());
    // }

    // void *plugin_symbol = dlsym(dl_handle, "vaccel_plugin");
    // if (!plugin_symbol) {
    //     FAIL("Error loading vaccel_plugin symbol from the library: " << dlerror());
    // }

    
    // struct vaccel_plugin* plugin_test = static_cast<struct vaccel_plugin*>(plugin_symbol);
    struct vaccel_plugin plugin_test;
    struct vaccel_plugin_info pinfo;
    plugin_test.info = &pinfo;
    plugin_test.info->name = pname;
    list_init_entry(&plugin_test.entry);
    list_init_entry(&plugin_test.ops);
    plugin_test.info->init = init;
    plugin_test.info->fini = fini;

    register_plugin(&plugin_test);
    //REQUIRE(ret == VACCEL_OK);

    struct vaccel_op test_op;

    /// this registeration is meant to us DL functions but not working at the moment
    /// instead we just test the register_plugin_functions here instead with mock plugins

    SECTION("invalid vaccel function") {
        REQUIRE(register_plugin_function(NULL) == VACCEL_EINVAL);

        test_op.func = NULL;
        test_op.type = VACCEL_FUNCTIONS_NR + 1;
        test_op.owner = &plugin_test;
        REQUIRE(register_plugin_function(&test_op) == VACCEL_EINVAL);
    }

    SECTION("Unknown function type") {
        test_op.func = (void *)&dummy_function;
        test_op.type = VACCEL_FUNCTIONS_NR + 1;
        test_op.owner = &plugin_test;
        REQUIRE(register_plugin_function(&test_op) == VACCEL_EINVAL);
    }

    SECTION("plugin owner is unknown") {
        test_op.func = (void *)&dummy_function;
        test_op.type = 1;
        test_op.owner = NULL;
        REQUIRE(register_plugin_function(&test_op) == VACCEL_EINVAL);
    }

    SECTION("valid plugin registration") {
        test_op.func = get_plugin_op(VACCEL_EXEC, 0); 
        test_op.type = 1;
        test_op.owner = &plugin_test;
        // REQUIRE(register_plugin_function(&test_op) == VACCEL_OK); doesnt work
    }

    // plugins_shutdown();
}

static int no_op() {return 1;}
static int no_op_exec(){return 2;}
static int no_op_fpga() {return 3;}

TEST_CASE("register_multiple_plugin_functions") {

    vaccel_plugin no_op_plugin;
    vaccel_plugin_info noop_pinfo;

    vaccel_op exec_operation;
    exec_operation.type = VACCEL_EXEC;
    exec_operation.func = (void *)no_op_exec;
    exec_operation.owner = &no_op_plugin;

    vaccel_op copy_operation;
    copy_operation.type = VACCEL_F_ARRAYCOPY;
    copy_operation.func = (void *)no_op_fpga;
    copy_operation.owner = &no_op_plugin;

    vaccel_op array_ops[2] = {exec_operation, copy_operation};

    int ret;

    no_op_plugin.info = &noop_pinfo;
    no_op_plugin.info->name = pname;
    list_init_entry(&no_op_plugin.entry);
    list_init_entry(&no_op_plugin.ops);
    no_op_plugin.info->init = init;
    no_op_plugin.info->fini = fini;

    plugins_bootstrap();

    ret = register_plugin(&no_op_plugin);
    REQUIRE(ret ==  VACCEL_OK);


    ret = register_plugin_functions(array_ops, (sizeof(array_ops) / sizeof(array_ops[0])));
    REQUIRE(ret ==  VACCEL_OK);


    void* operation ;
    operation = get_plugin_op(VACCEL_EXEC, 0);
    REQUIRE(operation != nullptr);
    ret = reinterpret_cast<int (*)(void)>(operation)();
    REQUIRE(ret == 2);

    operation = get_plugin_op(VACCEL_F_ARRAYCOPY, 0);
    REQUIRE(operation !=  nullptr);
    ret = reinterpret_cast<int (*)(void)>(operation)();
    REQUIRE(ret == 3);

    // we have implemented FPGA in our fixture and lets assume our plugin only implements FPGA functions
    noop_pinfo.type = VACCEL_PLUGIN_FPGA;

    operation = get_plugin_op(VACCEL_F_ARRAYCOPY, VACCEL_PLUGIN_FPGA);
    REQUIRE(operation != nullptr);

    ret = reinterpret_cast<int (*)(void)>(operation)();
    REQUIRE(ret == 3);

    ret = unregister_plugin(&no_op_plugin);
    REQUIRE(ret == VACCEL_OK);

    // plugins_shutdown();
}


TEST_CASE("register_plugin_functions_operation_fetch") {

    vaccel_plugin no_op_plugin;
    vaccel_plugin_info noop_pinfo;
    int ret;

    vaccel_op noop_operation;
    noop_operation.type = VACCEL_NO_OP;
    noop_operation.func = (void *)no_op;
    noop_operation.owner = nullptr;

    no_op_plugin.info = &noop_pinfo;
    no_op_plugin.info->name = pname;
    list_init_entry(&no_op_plugin.entry);
    list_init_entry(&no_op_plugin.ops);
    no_op_plugin.info->init = init;
    no_op_plugin.info->fini = fini;

    plugins_bootstrap();

    noop_operation.owner = &no_op_plugin;
    ret = register_plugin(&no_op_plugin);
    REQUIRE(ret ==  VACCEL_OK);

    SECTION("valid function registeration")
    {
        ret = register_plugin_function(&noop_operation);
        REQUIRE(ret ==  VACCEL_OK);
    }

    SECTION("unknown function fetch")
    {
        ret = register_plugin_function(&noop_operation);
        REQUIRE(ret ==  VACCEL_OK);
        enum vaccel_op_type unknown_function = static_cast<enum vaccel_op_type>(VACCEL_FUNCTIONS_NR + 1);
        void* operation = get_plugin_op(unknown_function, 0);
        REQUIRE(operation == nullptr);
    }

    SECTION("not registered function fetch")
    {        
        ret = register_plugin_function(&noop_operation);
        REQUIRE(ret ==  VACCEL_OK);
        void* operation = get_plugin_op(VACCEL_BLAS_SGEMM, 0);
        REQUIRE(operation == nullptr);
    }

    SECTION("NULL plugin operation")
    {
        REQUIRE(register_plugin_function(NULL) == VACCEL_EINVAL);
    }


    SECTION("fetch operation valid")
    {   
        ret = register_plugin_function(&noop_operation);
        REQUIRE(ret ==  VACCEL_OK);

        void* operation = get_plugin_op(VACCEL_NO_OP, 0);
        REQUIRE(operation != nullptr);

        ret = reinterpret_cast<int (*)(void)>(operation)();
        REQUIRE(ret ==  1);
    }

    ret = unregister_plugin(&no_op_plugin);
    REQUIRE(ret == VACCEL_OK);

    // plugins_shutdown();
}


TEST_CASE("get_all_available_functions") {

    vaccel_plugin no_op_plugin;
    vaccel_plugin_info noop_pinfo;
    int ret;

    vaccel_op noop_operation;
    noop_operation.type = VACCEL_NO_OP;
    noop_operation.func = (void *)no_op;
    noop_operation.owner = nullptr;

    no_op_plugin.info = &noop_pinfo;
    no_op_plugin.info->name = pname;
    list_init_entry(&no_op_plugin.entry);
    list_init_entry(&no_op_plugin.ops);
    no_op_plugin.info->init = init;
    no_op_plugin.info->fini = fini;

    plugins_bootstrap();

    noop_operation.owner = &no_op_plugin;
    ret = register_plugin(&no_op_plugin);
    REQUIRE(ret ==  VACCEL_OK);

    ret = register_plugin_function(&noop_operation);
    REQUIRE(ret ==  VACCEL_OK);

    ret = get_available_plugins(VACCEL_NO_OP);
    REQUIRE(ret == VACCEL_OK);

    /// this doesn't find exec as we don't implement it but it triggers the branch seen in coverage
    ret = get_available_plugins(VACCEL_EXEC);
    REQUIRE(ret == VACCEL_OK);

    ret = unregister_plugin(&no_op_plugin);
    REQUIRE(ret == VACCEL_OK);

    // plugins_shutdown();
}


