// SPDX-License-Identifier: Apache-2.0

/*
 *
 * The code below performs unit testing to resources.
 *
 * 1) plugins_bootstrap()
 * 2) register_plugin()
 * 3) register_plugin_function()
 * 4) get_available_plugins()
 * 5) unregister_plugin()
 * 6) plugins_shutdown()
 * 7) get_plugin_op()
 */

#include <catch.hpp>
#include <cerrno>
#include <cstring>
#include <utils.hpp>

#include <cstdlib>
#include <dlfcn.h>
#include <vaccel.h>

static const char *pname = "mock_plugin_test";
static const char *pversion = "0.0.0";

static auto fini() -> int
{
	return VACCEL_OK;
}
static auto init() -> int
{
	return VACCEL_OK;
}

static auto no_op() -> int
{
	return 2;
}
static auto exec_op() -> int
{
	return 3;
}

TEST_CASE("get_all_available_functions", "[core_plugin]")
{
	int ret;
	vaccel_plugin plugin;
	vaccel_plugin_info pinfo;
	plugin.dl_handle = nullptr;
	plugin.info = &pinfo;
	list_init_entry(&plugin.entry);
	list_init_entry(&plugin.ops);

	plugin.info->name = pname;
	plugin.info->version = pversion;
	plugin.info->vaccel_version = VACCEL_VERSION;
	plugin.info->init = init;
	plugin.info->fini = fini;
	plugin.info->is_virtio = false;
	plugin.info->sess_init = nullptr;
	plugin.info->sess_free = nullptr;
	plugin.info->type = VACCEL_PLUGIN_GENERIC;

	vaccel_op operation;
	operation.type = VACCEL_NO_OP;
	operation.func = (void *)no_op;
	operation.owner = &plugin;
	list_init_entry(&operation.plugin_entry);
	list_init_entry(&operation.func_entry);

	ret = plugins_bootstrap();
	REQUIRE(ret == VACCEL_OK);

	ret = register_plugin(&plugin);
	REQUIRE(ret == VACCEL_OK);

	ret = register_plugin_function(&operation);
	REQUIRE(ret == VACCEL_OK);

	ret = get_available_plugins(VACCEL_NO_OP);
	REQUIRE(ret == VACCEL_OK);

	ret = unregister_plugin(&plugin);
	REQUIRE(ret == VACCEL_OK);

	ret = plugins_shutdown();
	REQUIRE(ret == VACCEL_OK);
}

TEST_CASE("register_multiple_functions", "[core_plugin]")
{
	int ret;
	vaccel_plugin plugin;
	vaccel_plugin_info pinfo;
	plugin.dl_handle = nullptr;
	plugin.info = &pinfo;
	list_init_entry(&plugin.entry);
	list_init_entry(&plugin.ops);

	plugin.info->name = pname;
	plugin.info->version = pversion;
	plugin.info->vaccel_version = VACCEL_VERSION;
	plugin.info->init = init;
	plugin.info->fini = fini;
	plugin.info->is_virtio = false;
	plugin.info->sess_init = nullptr;
	plugin.info->sess_free = nullptr;
	plugin.info->type = VACCEL_PLUGIN_GENERIC;

	size_t operation_array_size = 2;

	vaccel_op operation1;
	operation1.type = VACCEL_NO_OP;
	operation1.func = (void *)no_op;
	operation1.owner = &plugin;
	list_init_entry(&operation1.plugin_entry);
	list_init_entry(&operation1.func_entry);

	vaccel_op operation2;
	operation2.type = VACCEL_EXEC;
	operation2.func = (void *)exec_op;
	operation2.owner = &plugin;
	list_init_entry(&operation2.plugin_entry);
	list_init_entry(&operation2.func_entry);

	vaccel_op operation_array[2] = { operation1, operation2 };

	ret = plugins_bootstrap();
	REQUIRE(ret == VACCEL_OK);

	ret = register_plugin(&plugin);
	REQUIRE(ret == VACCEL_OK);

	// fetch operation which is not registered yet
	void *operation;
	operation = get_plugin_op(VACCEL_EXEC, 0);
	REQUIRE(operation == nullptr);

	ret = register_plugin_functions(operation_array, operation_array_size);
	REQUIRE(ret == VACCEL_OK);

	operation = get_plugin_op(VACCEL_EXEC, 0);
	REQUIRE(operation != nullptr);
	ret = reinterpret_cast<int (*)()>(operation)();
	REQUIRE(ret == 3);

	operation = get_plugin_op(VACCEL_NO_OP, 0);
	REQUIRE(operation != nullptr);
	ret = reinterpret_cast<int (*)()>(operation)();
	REQUIRE(ret == 2);

	// search using hint
	operation = get_plugin_op(VACCEL_NO_OP, VACCEL_PLUGIN_GENERIC);
	REQUIRE(operation != nullptr);
	ret = reinterpret_cast<int (*)()>(operation)();
	REQUIRE(ret == 2);

	ret = unregister_plugin(&plugin);
	REQUIRE(ret == VACCEL_OK);

	ret = plugins_shutdown();
	REQUIRE(ret == VACCEL_OK);
}

TEST_CASE("register_plugin_ops", "[core_plugin]")
{
	int ret;
	vaccel_plugin plugin;
	vaccel_plugin_info pinfo;
	plugin.dl_handle = nullptr;
	plugin.info = &pinfo;
	list_init_entry(&plugin.entry);
	list_init_entry(&plugin.ops);

	plugin.info->name = pname;
	plugin.info->version = pversion;
	plugin.info->vaccel_version = VACCEL_VERSION;
	plugin.info->init = init;
	plugin.info->fini = fini;
	plugin.info->is_virtio = false;
	plugin.info->sess_init = nullptr;
	plugin.info->sess_free = nullptr;
	plugin.info->type = VACCEL_PLUGIN_GENERIC;

	vaccel_op operation;
	operation.type = VACCEL_NO_OP;
	operation.func = (void *)no_op;
	operation.owner = &plugin;
	list_init_entry(&operation.plugin_entry);
	list_init_entry(&operation.func_entry);

	SECTION("invalid_plugin")
	{
		ret = plugins_bootstrap();
		REQUIRE(ret == VACCEL_OK);

		ret = register_plugin(nullptr);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = plugins_shutdown();
		REQUIRE(ret == VACCEL_OK);
	}

	SECTION("not_bootstrapped_yet")
	{
		ret = register_plugin(&plugin);
		REQUIRE(ret == VACCEL_EBACKEND);

		ret = plugins_bootstrap();
		REQUIRE(ret == VACCEL_OK);

		ret = plugins_shutdown();
		REQUIRE(ret == VACCEL_OK);
	}

	SECTION("invalid_pinfo")
	{
		ret = plugins_bootstrap();
		REQUIRE(ret == VACCEL_OK);

		pinfo.fini = nullptr;
		ret = register_plugin(&plugin);
		REQUIRE(ret == VACCEL_EINVAL);

		pinfo.init = nullptr;
		ret = register_plugin(&plugin);
		REQUIRE(ret == VACCEL_EINVAL);

		pinfo.name = nullptr;
		ret = register_plugin(&plugin);
		REQUIRE(ret == VACCEL_EINVAL);

		pinfo.version = nullptr;
		ret = register_plugin(&plugin);
		REQUIRE(ret == VACCEL_EINVAL);

		pinfo.vaccel_version = nullptr;
		ret = register_plugin(&plugin);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = plugins_shutdown();
		REQUIRE(ret == VACCEL_OK);
	}
}

TEST_CASE("register_plugin_vaccel_versions", "[core_plugin]")
{
	int ret;
	int major;
	int minor1;
	int minor2;
	size_t vaccel_version_size;
	char *extra;
	char *vaccel_version;

	vaccel_plugin plugin;
	vaccel_plugin_info pinfo;
	plugin.dl_handle = nullptr;
	plugin.info = &pinfo;
	list_init_entry(&plugin.entry);
	list_init_entry(&plugin.ops);

	plugin.info->name = pname;
	plugin.info->version = pversion;
	plugin.info->vaccel_version = VACCEL_VERSION;
	plugin.info->init = init;
	plugin.info->fini = fini;
	plugin.info->is_virtio = false;
	plugin.info->sess_init = nullptr;
	plugin.info->sess_free = nullptr;
	plugin.info->type = VACCEL_PLUGIN_GENERIC;

	vaccel_op operation;
	operation.type = VACCEL_NO_OP;
	operation.func = (void *)no_op;
	operation.owner = &plugin;
	list_init_entry(&operation.plugin_entry);
	list_init_entry(&operation.func_entry);

	ret = parse_plugin_version(&major, &minor1, &minor2, &extra,
				   VACCEL_VERSION);
	REQUIRE(ret == VACCEL_OK);
	vaccel_version_size = strlen(VACCEL_VERSION) + 10;
	vaccel_version = (char *)calloc(1, vaccel_version_size);
	REQUIRE(vaccel_version);

	ret = plugins_bootstrap();
	REQUIRE(ret == VACCEL_OK);

	SECTION("same_version")
	{
		ret = register_plugin(&plugin);
		REQUIRE(ret == VACCEL_OK);
	}

	plugin.info->vaccel_version = vaccel_version;
	SECTION("same_old_format_version")
	{
		snprintf(vaccel_version, vaccel_version_size, "v%d.%d.%d%s",
			 major, minor1, minor2, extra);
		ret = register_plugin(&plugin);
		REQUIRE(ret == VACCEL_OK);
	}

	SECTION("different_major_version")
	{
		snprintf(vaccel_version, vaccel_version_size, "%d.%d.%d%s",
			 major + 1, minor1, minor2, extra);
		ret = register_plugin(&plugin);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	SECTION("different_minor1_version")
	{
		snprintf(vaccel_version, vaccel_version_size, "%d.%d.%d%s",
			 major, minor1 + 1, minor2, extra);
		ret = register_plugin(&plugin);
		REQUIRE(ret == VACCEL_OK);
	}

	SECTION("different_minor2_version")
	{
		snprintf(vaccel_version, vaccel_version_size, "%d.%d.%d%s",
			 major, minor1, minor2 + 1, extra);
		ret = register_plugin(&plugin);
		REQUIRE(ret == VACCEL_OK);
	}

	SECTION("different_extra_version")
	{
		snprintf(vaccel_version, vaccel_version_size, "%d.%d.%d%s",
			 major, minor1, minor2 + 1, "-extra");
		ret = register_plugin(&plugin);
		REQUIRE(ret == VACCEL_OK);
	}

	SECTION("wrong_format_version")
	{
		snprintf(vaccel_version, vaccel_version_size, "%d-%d.%d%s",
			 major, minor1, minor2, extra);
		ret = register_plugin(&plugin);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	SECTION("wrong_format_version_ignore")
	{
		snprintf(vaccel_version, vaccel_version_size, "%d-%d.%d%s",
			 major, minor1, minor2, extra);
		ret = setenv("VACCEL_IGNORE_VERSION", "1", 1);
		REQUIRE(ret == 0);

		ret = register_plugin(&plugin);
		REQUIRE(ret == VACCEL_OK);
	}

	ret = plugins_shutdown();
	REQUIRE(ret == VACCEL_OK);

	free(vaccel_version);
	free(extra);
}
