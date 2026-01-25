// SPDX-License-Identifier: Apache-2.0

/*
 * The code below performs unit testing to plugin.
 *
 * 1) plugin_parse_version()
 * 2) plugin_register()
 * 3) plugin_unregister()
 * 4) vaccel_plugin_register_ops()
 * 5) vaccel_plugin_register_op()
 * 6) plugin_find()
 * 7) plugin_find_by_name()
 * 8) plugin_get_op_func()
 * 9) plugin_count()
 * 10) vaccel_plugin_load()
 * 11) vaccel_plugin_parse_and_load()
 *
 */

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include "utils.hpp"
#include "vaccel.h"
#include <catch2/catch_test_macros.hpp>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <pthread.h>
#include <unistd.h>

static const char *pname = "mock_plugin_test";
static const char *pversion = "0.0.0";

static auto init() -> int
{
	return VACCEL_OK;
}
static auto fini() -> int
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

static auto mock_plugin_new() -> struct vaccel_plugin *
{
	auto *pinfo = (struct vaccel_plugin_info *)calloc(
		1, sizeof(struct vaccel_plugin_info));
	if (pinfo == nullptr)
		return nullptr;

	pinfo->name = pname;
	pinfo->version = pversion;
	pinfo->vaccel_version = VACCEL_VERSION;
	pinfo->init = init;
	pinfo->fini = fini;
	pinfo->is_virtio = false;
	pinfo->type = VACCEL_PLUGIN_GENERIC;
	pinfo->session_init = nullptr;
	pinfo->session_release = nullptr;
	pinfo->session_update = nullptr;
	pinfo->resource_register = nullptr;
	pinfo->resource_unregister = nullptr;
	pinfo->resource_sync = nullptr;

	auto *plugin =
		(struct vaccel_plugin *)calloc(1, sizeof(struct vaccel_plugin));
	if (plugin == nullptr) {
		free(pinfo);
		return nullptr;
	}

	plugin->dl_handle = nullptr;
	plugin->entry = LIST_ENTRY_INIT(plugin->entry);
	plugin->info = pinfo;
	for (auto &op : plugin->ops)
		op = nullptr;

	return plugin;
}

static auto mock_plugin_delete(struct vaccel_plugin *plugin) -> void
{
	if (plugin == nullptr)
		return;

	if (plugin->info != nullptr)
		free(plugin->info);

	free(plugin);
}

TEST_CASE("plugin_parse_version", "[core][plugin]")
{
	int ret;
	int major = -1;
	int minor1 = -1;
	int minor2 = -1;
	size_t vaccel_version_size;
	char *extra = nullptr;
	char *vaccel_version = nullptr;

	vaccel_version_size = strlen(VACCEL_VERSION) + 10;
	vaccel_version = (char *)calloc(1, vaccel_version_size);
	REQUIRE(vaccel_version != nullptr);

	SECTION("success")
	{
		snprintf(vaccel_version, vaccel_version_size, "v%d.%d.%d%s", 1,
			 2, 3, "extra");

		ret = plugin_parse_version(&major, &minor1, &minor2, &extra,
					   vaccel_version);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(major == 1);
		REQUIRE(minor1 == 2);
		REQUIRE(minor2 == 3);
		REQUIRE(strcmp(extra, "extra") == 0);
		free(extra);
	}

	SECTION("success_no_extra")
	{
		snprintf(vaccel_version, vaccel_version_size, "v%d.%d.%d", 1, 2,
			 3);

		ret = plugin_parse_version(&major, &minor1, &minor2, &extra,
					   vaccel_version);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(major == 1);
		REQUIRE(minor1 == 2);
		REQUIRE(minor2 == 3);
		REQUIRE(extra == nullptr);
	}

	SECTION("invalid_version")
	{
		snprintf(vaccel_version, vaccel_version_size, "v%d.%d", 1, 2);

		ret = plugin_parse_version(&major, &minor1, &minor2, &extra,
					   vaccel_version);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(major == -1);
		REQUIRE(minor1 == -1);
		REQUIRE(minor2 == -1);
		REQUIRE(extra == nullptr);
	}

	SECTION("null_arguments")
	{
		snprintf(vaccel_version, vaccel_version_size, "v%d.%d.%d", 1, 2,
			 3);

		ret = plugin_parse_version(&major, &minor1, &minor2, &extra,
					   nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(major == -1);
		REQUIRE(minor1 == -1);
		REQUIRE(minor2 == -1);
		REQUIRE(extra == nullptr);
	}

	free(vaccel_version);
}

TEST_CASE("plugin_register", "[core][plugin]")
{
	int ret;
	struct vaccel_plugin *plugin = mock_plugin_new();
	REQUIRE(plugin != nullptr);

	REQUIRE(plugins_cleanup() == VACCEL_OK);

	SECTION("not_bootstrapped_yet")
	{
		ret = plugin_register(plugin);
		REQUIRE(ret == VACCEL_EBACKEND);
	}

	REQUIRE(plugins_bootstrap() == VACCEL_OK);

	SECTION("null_arguments")
	{
		ret = plugin_register(nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	ret = plugin_register(plugin);
	REQUIRE(ret == VACCEL_OK);

	REQUIRE(plugin_count() == 1);

	SECTION("already_registered")
	{
		ret = plugin_register(plugin);
		REQUIRE(ret == VACCEL_EEXIST);
	}

	REQUIRE(plugin_unregister(plugin) == VACCEL_OK);

	SECTION("invalid_name")
	{
		plugin->info->name = nullptr;
		ret = plugin_register(plugin);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	SECTION("invalid_version")
	{
		plugin->info->version = nullptr;
		ret = plugin_register(plugin);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	SECTION("invalid_init")
	{
		plugin->info->init = nullptr;
		ret = plugin_register(plugin);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	SECTION("invalid_fini")
	{
		plugin->info->fini = nullptr;
		ret = plugin_register(plugin);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	SECTION("invalid_vaccel_version")
	{
		plugin->info->vaccel_version = nullptr;
		ret = plugin_register(plugin);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	mock_plugin_delete(plugin);
}

TEST_CASE("plugin_register_vaccel_versions", "[core][plugin]")
{
	int ret;
	int major;
	int minor1;
	int minor2;
	size_t vaccel_version_size;
	char *extra;
	char *vaccel_version;
	struct vaccel_plugin *plugin = mock_plugin_new();
	REQUIRE(plugin != nullptr);

	REQUIRE(plugin_parse_version(&major, &minor1, &minor2, &extra,
				     VACCEL_VERSION) == VACCEL_OK);

	vaccel_version_size = strlen(VACCEL_VERSION) + 10;
	vaccel_version = (char *)calloc(1, vaccel_version_size);
	REQUIRE(vaccel_version);

	SECTION("same_version")
	{
		ret = plugin_register(plugin);
		REQUIRE(ret == VACCEL_OK);

		REQUIRE(plugin_unregister(plugin) == VACCEL_OK);
	}

	plugin->info->vaccel_version = vaccel_version;

	SECTION("same_old_format_version")
	{
		snprintf(vaccel_version, vaccel_version_size, "v%d.%d.%d%s",
			 major, minor1, minor2, extra);
		ret = plugin_register(plugin);
		REQUIRE(ret == VACCEL_OK);

		REQUIRE(plugin_unregister(plugin) == VACCEL_OK);
	}

	SECTION("no_extra_version")
	{
		snprintf(vaccel_version, vaccel_version_size, "v%d.%d.%d",
			 major, minor1, minor2);
		ret = plugin_register(plugin);
		REQUIRE(ret == VACCEL_OK);

		REQUIRE(plugin_unregister(plugin) == VACCEL_OK);
	}

	SECTION("different_major_version")
	{
		snprintf(vaccel_version, vaccel_version_size, "%d.%d.%d%s",
			 major + 1, minor1, minor2, extra);
		ret = plugin_register(plugin);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	SECTION("different_minor1_version")
	{
		snprintf(vaccel_version, vaccel_version_size, "%d.%d.%d%s",
			 major, minor1 + 1, minor2, extra);
		ret = plugin_register(plugin);
		REQUIRE(ret == VACCEL_OK);

		REQUIRE(plugin_unregister(plugin) == VACCEL_OK);
	}

	SECTION("different_minor2_version")
	{
		snprintf(vaccel_version, vaccel_version_size, "%d.%d.%d%s",
			 major, minor1, minor2 + 1, extra);
		ret = plugin_register(plugin);
		REQUIRE(ret == VACCEL_OK);

		REQUIRE(plugin_unregister(plugin) == VACCEL_OK);
	}

	SECTION("different_extra_version")
	{
		snprintf(vaccel_version, vaccel_version_size, "%d.%d.%d%s",
			 major, minor1, minor2 + 1, "-extra");
		ret = plugin_register(plugin);
		REQUIRE(ret == VACCEL_OK);

		REQUIRE(plugin_unregister(plugin) == VACCEL_OK);
	}

	SECTION("wrong_format_version")
	{
		snprintf(vaccel_version, vaccel_version_size, "%d-%d.%d%s",
			 major, minor1, minor2, extra);
		ret = plugin_register(plugin);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	SECTION("wrong_format_version_ignore")
	{
		snprintf(vaccel_version, vaccel_version_size, "%d-%d.%d%s",
			 major, minor1, minor2, extra);
		ret = plugin_register(plugin);
		REQUIRE(ret == VACCEL_EINVAL);

		auto *config = (struct vaccel_config *)vaccel_config();
		config->version_ignore = true;

		ret = plugin_register(plugin);
		REQUIRE(ret == VACCEL_OK);

		config->version_ignore = false;
		REQUIRE(plugin_unregister(plugin) == VACCEL_OK);
	}

	mock_plugin_delete(plugin);
	free(vaccel_version);
	free(extra);
}

TEST_CASE("plugin_unregister", "[core][plugin]")
{
	int ret;
	struct vaccel_plugin *plugin = mock_plugin_new();
	REQUIRE(plugin != nullptr);

	REQUIRE(plugins_cleanup() == VACCEL_OK);

	SECTION("not_bootstrapped_yet")
	{
		ret = plugin_unregister(plugin);
		REQUIRE(ret == VACCEL_EBACKEND);
	}

	REQUIRE(plugins_bootstrap() == VACCEL_OK);

	SECTION("not_registered")
	{
		ret = plugin_unregister(plugin);
		REQUIRE(ret == VACCEL_ENOENT);
	}

	REQUIRE(plugin_register(plugin) == VACCEL_OK);

	SECTION("null_arguments")
	{
		ret = plugin_unregister(nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	SECTION("missing_plugin_info")
	{
		struct vaccel_plugin_info *pinfo = plugin->info;
		plugin->info = nullptr;

		ret = plugin_unregister(plugin);
		REQUIRE(ret == VACCEL_EINVAL);

		plugin->info = pinfo;
	}

	ret = plugin_unregister(plugin);
	REQUIRE(ret == VACCEL_OK);

	REQUIRE(plugin_count() == 0);

	mock_plugin_delete(plugin);
}

TEST_CASE("vaccel_plugin_register_ops", "[core][plugin]")
{
	int ret;
	struct vaccel_plugin *plugin = mock_plugin_new();
	REQUIRE(plugin != nullptr);

	REQUIRE(plugin_register(plugin) == VACCEL_OK);

	vaccel_op op1 = { .type = VACCEL_OP_NOOP,
			  .func = (void *)no_op,
			  .owner = plugin };
	vaccel_op const op2 = { .type = VACCEL_OP_EXEC,
				.func = (void *)exec_op,
				.owner = plugin };

	const size_t ops_size = 2;
	vaccel_op ops[ops_size] = { op1, op2 };

	SECTION("invalid_arguments")
	{
		ret = vaccel_plugin_register_ops(nullptr, ops_size);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(plugin->ops[VACCEL_OP_NOOP] == nullptr);
		REQUIRE(plugin->ops[VACCEL_OP_EXEC] == nullptr);

		ret = vaccel_plugin_register_ops(ops, 0);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(plugin->ops[VACCEL_OP_NOOP] == nullptr);
		REQUIRE(plugin->ops[VACCEL_OP_EXEC] == nullptr);

		ret = vaccel_plugin_register_ops(ops, VACCEL_OP_MAX + 1);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(plugin->ops[VACCEL_OP_NOOP] == nullptr);
		REQUIRE(plugin->ops[VACCEL_OP_EXEC] == nullptr);
	}

	ret = vaccel_plugin_register_ops(ops, ops_size);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(plugin->ops[VACCEL_OP_NOOP] == &ops[0]);
	REQUIRE(plugin->ops[VACCEL_OP_EXEC] == &ops[1]);

	ret = vaccel_plugin_register_op(&op1);
	REQUIRE(ret == VACCEL_OK);

	REQUIRE(plugin_unregister(plugin) == VACCEL_OK);
	mock_plugin_delete(plugin);
}

TEST_CASE("plugin_find", "[core][plugin]")
{
	struct vaccel_plugin *plugin = nullptr;

	struct vaccel_plugin *accel_plugin = mock_plugin_new();
	REQUIRE(accel_plugin != nullptr);

	struct vaccel_plugin *virtio_plugin = mock_plugin_new();
	REQUIRE(virtio_plugin != nullptr);
	virtio_plugin->info->is_virtio = true;

	SECTION("not_registered")
	{
		plugin = plugin_find(0);
		REQUIRE(plugin == nullptr);
	}

	REQUIRE(plugin_register(accel_plugin) == VACCEL_OK);
	REQUIRE(plugin_count() == 1);

	plugin = plugin_find(0);
	REQUIRE(plugin == accel_plugin);

	SECTION("virtio_not_registered")
	{
		plugin = plugin_find(VACCEL_PLUGIN_REMOTE);
		REQUIRE(plugin == nullptr);
	}

	REQUIRE(plugin_register(virtio_plugin) == VACCEL_OK);
	REQUIRE(plugin_count() == 2);

	plugin = plugin_find(0 | VACCEL_PLUGIN_REMOTE);
	REQUIRE(plugin == virtio_plugin);

	SECTION("fallback")
	{
		plugin = plugin_find(VACCEL_PLUGIN_GPU);
		REQUIRE(plugin == accel_plugin);
	}

	REQUIRE(plugin_unregister(accel_plugin) == VACCEL_OK);
	REQUIRE(plugin_unregister(virtio_plugin) == VACCEL_OK);

	mock_plugin_delete(accel_plugin);
	mock_plugin_delete(virtio_plugin);
}

TEST_CASE("plugin_find_by_name", "[core][plugin]")
{
	struct vaccel_plugin *plugin = nullptr;

	struct vaccel_plugin *accel_plugin = mock_plugin_new();
	REQUIRE(accel_plugin != nullptr);

	SECTION("not_registered")
	{
		plugin = plugin_find_by_name("mock_plugin_test");
		REQUIRE(plugin == nullptr);
	}

	REQUIRE(plugin_register(accel_plugin) == VACCEL_OK);
	REQUIRE(plugin_count() == 1);

	plugin = plugin_find_by_name("mock_plugin_test");
	REQUIRE(plugin == accel_plugin);

	SECTION("invalid_arguments")
	{
		plugin = plugin_find_by_name(nullptr);
		REQUIRE(plugin == nullptr);
	}

	REQUIRE(plugin_unregister(accel_plugin) == VACCEL_OK);
	mock_plugin_delete(accel_plugin);
}

TEST_CASE("plugin_get_op_func", "[core][plugin]")
{
	int ret;
	void *op = nullptr;
	struct vaccel_plugin *plugin = mock_plugin_new();
	REQUIRE(plugin != nullptr);

	REQUIRE(plugin_register(plugin) == VACCEL_OK);

	SECTION("operation_not_registered")
	{
		op = plugin_get_op_func(plugin, VACCEL_OP_EXEC);
		REQUIRE(op == nullptr);
	}

	vaccel_op const op1 = { .type = VACCEL_OP_NOOP,
				.func = (void *)no_op,
				.owner = plugin };
	vaccel_op const op2 = { .type = VACCEL_OP_EXEC,
				.func = (void *)exec_op,
				.owner = plugin };

	const size_t ops_size = 2;
	vaccel_op ops[ops_size] = { op1, op2 };

	REQUIRE(vaccel_plugin_register_ops(ops, ops_size) == VACCEL_OK);

	SECTION("invalid_arguments")
	{
		op = plugin_get_op_func(plugin, VACCEL_OP_MAX);
		REQUIRE(op == nullptr);

		op = plugin_get_op_func(nullptr, VACCEL_OP_NOOP);
		REQUIRE(op == nullptr);
	}

	op = plugin_get_op_func(plugin, VACCEL_OP_EXEC);
	REQUIRE(op != nullptr);
	ret = reinterpret_cast<int (*)()>(op)();
	REQUIRE(ret == 3);

	op = plugin_get_op_func(plugin, VACCEL_OP_NOOP);
	REQUIRE(op != nullptr);
	ret = reinterpret_cast<int (*)()>(op)();
	REQUIRE(ret == 2);

	REQUIRE(plugin_unregister(plugin) == VACCEL_OK);
	mock_plugin_delete(plugin);
}

TEST_CASE("vaccel_plugin_load", "[core][plugin]")
{
	int ret;
	char *plugin_noop_path =
		abs_path(BUILD_ROOT, "plugins/noop/libvaccel-noop.so");
	char *lib_path = abs_path(BUILD_ROOT, "examples/libmytestlib.so");

	REQUIRE(fs_path_is_file(plugin_noop_path));

	REQUIRE(plugins_cleanup() == VACCEL_OK);

	SECTION("plugins_not_initialized")
	{
		ret = vaccel_plugin_load(plugin_noop_path);
		REQUIRE(ret == VACCEL_EBACKEND);
	}

	REQUIRE(plugins_bootstrap() == VACCEL_OK);

	SECTION("null_arguments")
	{
		ret = vaccel_plugin_load(nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	SECTION("non_existent_file")
	{
		ret = vaccel_plugin_load("some/random/path.so");
		REQUIRE(ret == VACCEL_ENOENT);
	}

	SECTION("non_plugin_lib")
	{
		ret = vaccel_plugin_load(lib_path);
		REQUIRE(ret == VACCEL_ELIBBAD);
	}

	SECTION("success")
	{
		ret = vaccel_plugin_load(plugin_noop_path);
		REQUIRE(ret == VACCEL_OK);

		REQUIRE(plugin_count() == 1);

		struct vaccel_plugin *plugin = plugin_find(0);
		REQUIRE(plugin != nullptr);
		REQUIRE(plugin->dl_handle != nullptr);
		REQUIRE(strcmp(plugin->info->name, "noop") == 0);
	}

	/* Clean up loaded plugins for other tests */
	REQUIRE(plugins_cleanup() == VACCEL_OK);
	REQUIRE(plugins_bootstrap() == VACCEL_OK);

	free(plugin_noop_path);
	free(lib_path);
}

TEST_CASE("vaccel_plugin_load_and_exec_func", "[core][plugin]")
{
	int ret;
	struct vaccel_session session;
	char *plugin_noop_path =
		abs_path(BUILD_ROOT, "plugins/noop/libvaccel-noop.so");
	char *plugin_exec_path =
		abs_path(BUILD_ROOT, "plugins/exec/libvaccel-exec.so");
	char *lib_path = abs_path(BUILD_ROOT, "examples/libmytestlib.so");

	REQUIRE(fs_path_is_file(plugin_noop_path));
	REQUIRE(fs_path_is_file(plugin_exec_path));

	struct vaccel_arg_array read_args;
	struct vaccel_arg_array write_args;
	REQUIRE(vaccel_arg_array_init(&read_args, 1) == VACCEL_OK);
	REQUIRE(vaccel_arg_array_init(&write_args, 1) == VACCEL_OK);

	int32_t input = 20;
	int32_t output = 0;
	REQUIRE(vaccel_arg_array_add_int32(&read_args, &input) == VACCEL_OK);
	REQUIRE(vaccel_arg_array_add_int32(&write_args, &output) == VACCEL_OK);

	/* Load noop plugin */
	ret = vaccel_plugin_load(plugin_noop_path);
	REQUIRE(ret == VACCEL_OK);

	REQUIRE(vaccel_session_init(&session, 0) == VACCEL_OK);

	/* noop implementation - output should be equal to input */
	REQUIRE(vaccel_exec(&session, lib_path, "mytestfunc", read_args.args,
			    read_args.count, write_args.args,
			    write_args.count) == VACCEL_OK);
	REQUIRE(output == input);

	output = 0;

	/* Load exec plugin */
	ret = vaccel_plugin_load(plugin_exec_path);
	REQUIRE(ret == VACCEL_OK);

	/* Update hint - prefer exec plugin */
	REQUIRE(vaccel_session_update(&session, VACCEL_PLUGIN_SOFTWARE) ==
		VACCEL_OK);

	/* exec implementation -  the output should be 2 * input */
	REQUIRE(vaccel_exec(&session, lib_path, "mytestfunc", read_args.args,
			    read_args.count, write_args.args,
			    write_args.count) == VACCEL_OK);
	REQUIRE(output == (2 * input));

	REQUIRE(vaccel_session_release(&session) == VACCEL_OK);

	REQUIRE(vaccel_arg_array_release(&read_args) == VACCEL_OK);
	REQUIRE(vaccel_arg_array_release(&write_args) == VACCEL_OK);

	/* Clean up loaded plugins for other tests */
	REQUIRE(plugins_cleanup() == VACCEL_OK);
	REQUIRE(plugins_bootstrap() == VACCEL_OK);

	free(plugin_exec_path);
	free(plugin_noop_path);
	free(lib_path);
}

TEST_CASE("vaccel_plugin_parse_and_load", "[core][plugin]")
{
	int ret;
	char *plugin_noop_path =
		abs_path(BUILD_ROOT, "plugins/noop/libvaccel-noop.so");
	char *plugin_exec_path =
		abs_path(BUILD_ROOT, "plugins/exec/libvaccel-exec.so");
	char *plugin_paths = nullptr;

	REQUIRE(fs_path_is_file(plugin_noop_path));
	REQUIRE(fs_path_is_file(plugin_exec_path));

	REQUIRE(plugins_cleanup() == VACCEL_OK);

	SECTION("plugins_not_initialized")
	{
		ret = vaccel_plugin_parse_and_load(plugin_noop_path);
		REQUIRE(ret == VACCEL_EBACKEND);
	}

	REQUIRE(plugins_bootstrap() == VACCEL_OK);

	SECTION("null_arguments")
	{
		ret = vaccel_plugin_parse_and_load(nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	SECTION("success")
	{
		size_t const plugin_noop_path_len =
			strlen(plugin_noop_path) + 1;
		size_t const plugin_exec_path_len =
			strlen(plugin_exec_path) + 1;

		plugin_paths = (char *)calloc(1, plugin_noop_path_len +
							 plugin_exec_path_len);
		REQUIRE(plugin_paths != nullptr);
		strncpy(plugin_paths, plugin_noop_path, plugin_noop_path_len);
		plugin_paths[plugin_noop_path_len - 1] = ':';
		strncat(plugin_paths, plugin_exec_path, plugin_exec_path_len);

		ret = vaccel_plugin_parse_and_load(plugin_paths);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(plugin_count() == 2);

		free(plugin_paths);
	}

	/* Clean up loaded plugins for other tests */
	REQUIRE(plugins_cleanup() == VACCEL_OK);
	REQUIRE(plugins_bootstrap() == VACCEL_OK);

	free(plugin_exec_path);
	free(plugin_noop_path);
}

enum {
	TEST_THREADS_NUM = 50,
	TEST_THREAD_REG_NUM = 30,
};

struct thread_data {
	size_t id;
	struct vaccel_plugin *plugin;
};

static auto register_and_unregister_plugin(void *arg) -> void *
{
	auto *data = (struct thread_data *)arg;
	struct vaccel_plugin *plugin = data->plugin;

	for (size_t i = 0; i < TEST_THREAD_REG_NUM; i++) {
		REQUIRE(plugin_register(plugin) == VACCEL_OK);
		printf("Thread %zu: Registered plugin %s\n", data->id,
		       plugin->info->name);

		// Add random delay to simulate work
		usleep(rand() % 1000);

		REQUIRE(plugin_count() >= 1);
		REQUIRE(plugin_find(0) != nullptr);

		REQUIRE(plugin_unregister(plugin) == VACCEL_OK);
		printf("Thread %zu: Unregistered plugin %s\n", data->id,
		       plugin->info->name);
	}
	return nullptr;
}

TEST_CASE("resource_init_find_and_release_concurrent", "[core][resource]")
{
	pthread_t threads[TEST_THREADS_NUM];
	struct thread_data thread_data[TEST_THREADS_NUM];
	struct vaccel_plugin *plugins[TEST_THREADS_NUM];

	for (auto &plugin : plugins) {
		plugin = mock_plugin_new();
		REQUIRE(plugin != nullptr);
	}

	for (size_t i = 0; i < TEST_THREADS_NUM; i++) {
		thread_data[i].id = i;
		thread_data[i].plugin = plugins[i];
		pthread_create(&threads[i], nullptr,
			       register_and_unregister_plugin, &thread_data[i]);
	}

	for (unsigned long const thread : threads)
		pthread_join(thread, nullptr);

	for (auto &plugin : plugins)
		mock_plugin_delete(plugin);
}
