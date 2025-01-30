// SPDX-License-Identifier: Apache-2.0

/*
 * The code below performs unit testing to config.
 *
 * 1) vaccel_config_init_from_env()
 * 2) vaccel_config_init_from()
 * 3) vaccel_config_from_env()
 * 4) vaccel_config_from()
 * 5) vaccel_config_init()
 * 6) vaccel_config_release()
 * 7) vaccel_config_new()
 * 8) vaccel_config_delete()
 * 9) config_print_debug()
 *
 */

#include "vaccel.h"
#include <catch.hpp>
#include <cstdlib>
#include <cstring>

TEST_CASE("vaccel_config_init_from_env", "[core][config]")
{
	int ret;
	const char *plugins_env = getenv(CONFIG_PLUGINS_ENV);
	const char *log_level_env_str = getenv(CONFIG_LOG_LEVEL_ENV);
	vaccel_log_level_t const log_level_env =
		(log_level_env_str != nullptr) ?
			(vaccel_log_level_t)atoi(log_level_env_str) :
			CONFIG_LOG_LEVEL_DEFAULT;
	const char *log_file_env = getenv(CONFIG_LOG_FILE_ENV);
	const char *profiling_enabled_env_str =
		getenv(CONFIG_PROFILING_ENABLED_ENV);
	bool const profiling_enabled_env =
		(profiling_enabled_env_str != nullptr) ?
			(atoi(profiling_enabled_env_str) != 0) :
			CONFIG_PROFILING_ENABLED_DEFAULT;
	const char *version_ignore_env_str = getenv(CONFIG_VERSION_IGNORE_ENV);
	bool const version_ignore_env =
		(version_ignore_env_str != nullptr) ?
			(atoi(version_ignore_env_str) != 0) :
			CONFIG_VERSION_IGNORE_DEFAULT;
	struct vaccel_config config = {
		.plugins = CONFIG_PLUGINS_DEFAULT,
		.log_level = CONFIG_LOG_LEVEL_DEFAULT,
		.log_file = CONFIG_LOG_FILE_DEFAULT,
		.profiling_enabled = CONFIG_PROFILING_ENABLED_DEFAULT,
		.version_ignore = CONFIG_VERSION_IGNORE_DEFAULT
	};

	SECTION("success")
	{
		ret = vaccel_config_init_from_env(&config);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE((config.plugins == nullptr ||
			 strcmp(config.plugins, plugins_env) == 0));
		REQUIRE(config.log_level == log_level_env);
		REQUIRE((config.log_file == nullptr ||
			 strcmp(config.log_file, log_file_env) == 0));
		REQUIRE(config.profiling_enabled == profiling_enabled_env);
		REQUIRE(config.version_ignore == version_ignore_env);

		REQUIRE(vaccel_config_release(&config) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_config_init_from_env(nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(config.plugins == CONFIG_PLUGINS_DEFAULT);
		REQUIRE(config.log_level == CONFIG_LOG_LEVEL_DEFAULT);
		REQUIRE(config.log_file == CONFIG_LOG_FILE_DEFAULT);
		REQUIRE(config.profiling_enabled ==
			CONFIG_PROFILING_ENABLED_DEFAULT);
		REQUIRE(config.version_ignore == CONFIG_VERSION_IGNORE_DEFAULT);
	}
}

TEST_CASE("vaccel_config_init_from", "[core][config]")
{
	int ret;
	struct vaccel_config config_env;
	struct vaccel_config config = {
		.plugins = CONFIG_PLUGINS_DEFAULT,
		.log_level = CONFIG_LOG_LEVEL_DEFAULT,
		.log_file = CONFIG_LOG_FILE_DEFAULT,
		.profiling_enabled = CONFIG_PROFILING_ENABLED_DEFAULT,
		.version_ignore = CONFIG_VERSION_IGNORE_DEFAULT
	};

	REQUIRE(vaccel_config_init_from_env(&config_env) == VACCEL_OK);

	SECTION("success")
	{
		ret = vaccel_config_init_from(&config, &config_env);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE((config.plugins == config_env.plugins ||
			 strcmp(config.plugins, config_env.plugins) == 0));
		REQUIRE(config.log_level == config_env.log_level);
		REQUIRE((config.log_file == config_env.log_file ||
			 strcmp(config.log_file, config_env.log_file) == 0));
		REQUIRE(config.profiling_enabled ==
			config_env.profiling_enabled);
		REQUIRE(config.version_ignore == config_env.version_ignore);

		REQUIRE(vaccel_config_release(&config) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_config_init_from(nullptr, &config_env);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(config.plugins == CONFIG_PLUGINS_DEFAULT);
		REQUIRE(config.log_level == CONFIG_LOG_LEVEL_DEFAULT);
		REQUIRE(config.log_file == CONFIG_LOG_FILE_DEFAULT);
		REQUIRE(config.profiling_enabled ==
			CONFIG_PROFILING_ENABLED_DEFAULT);
		REQUIRE(config.version_ignore == CONFIG_VERSION_IGNORE_DEFAULT);

		ret = vaccel_config_init_from(&config, nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(config.plugins == CONFIG_PLUGINS_DEFAULT);
		REQUIRE(config.log_level == CONFIG_LOG_LEVEL_DEFAULT);
		REQUIRE(config.log_file == CONFIG_LOG_FILE_DEFAULT);
		REQUIRE(config.profiling_enabled ==
			CONFIG_PROFILING_ENABLED_DEFAULT);
		REQUIRE(config.version_ignore == CONFIG_VERSION_IGNORE_DEFAULT);
	}

	REQUIRE(vaccel_config_release(&config_env) == VACCEL_OK);
}

TEST_CASE("vaccel_config_from_env", "[core][config]")
{
	int ret;
	struct vaccel_config *config = nullptr;

	SECTION("success")
	{
		ret = vaccel_config_from_env(&config);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(config != nullptr);

		REQUIRE(vaccel_config_delete(config) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_config_from_env(nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(config == nullptr);
	}
}

TEST_CASE("vaccel_config_from", "[core][config]")
{
	int ret;
	struct vaccel_config *config = nullptr;
	struct vaccel_config config_env;

	REQUIRE(vaccel_config_init_from_env(&config_env) == VACCEL_OK);

	SECTION("success")
	{
		ret = vaccel_config_from(&config, &config_env);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(config != nullptr);

		REQUIRE(vaccel_config_delete(config) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_config_from(nullptr, &config_env);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(config == nullptr);

		ret = vaccel_config_from(&config, nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(config == nullptr);
	}

	REQUIRE(vaccel_config_release(&config_env) == VACCEL_OK);
}

TEST_CASE("vaccel_config_init", "[core][config]")
{
	int ret;
	const char *plugins = "libvaccel-noop.so:libvaccel-exec.so";
	vaccel_log_level_t const log_level = VACCEL_LOG_DEBUG;
	const char *log_file = "vaccel_log";
	bool const profiling_enabled = true;
	bool const version_ignore = true;
	struct vaccel_config config = {
		.plugins = CONFIG_PLUGINS_DEFAULT,
		.log_level = CONFIG_LOG_LEVEL_DEFAULT,
		.log_file = CONFIG_LOG_FILE_DEFAULT,
		.profiling_enabled = CONFIG_PROFILING_ENABLED_DEFAULT,
		.version_ignore = CONFIG_VERSION_IGNORE_DEFAULT
	};

	SECTION("success")
	{
		ret = vaccel_config_init(&config, plugins, log_level, log_file,
					 profiling_enabled, version_ignore);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(strcmp(config.plugins, plugins) == 0);
		REQUIRE(config.log_level == log_level);
		REQUIRE(strcmp(config.log_file, log_file) == 0);
		REQUIRE(config.profiling_enabled == profiling_enabled);
		REQUIRE(config.version_ignore == version_ignore);

		REQUIRE(vaccel_config_release(&config) == VACCEL_OK);
	}

	SECTION("success with null strings")
	{
		ret = vaccel_config_init(&config, nullptr, log_level, nullptr,
					 profiling_enabled, version_ignore);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(config.plugins == nullptr);
		REQUIRE(config.log_level == log_level);
		REQUIRE(config.log_file == nullptr);
		REQUIRE(config.profiling_enabled == profiling_enabled);
		REQUIRE(config.version_ignore == version_ignore);

		REQUIRE(vaccel_config_release(&config) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_config_init(nullptr, plugins, log_level, log_file,
					 profiling_enabled, version_ignore);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(config.plugins == CONFIG_PLUGINS_DEFAULT);
		REQUIRE(config.log_level == CONFIG_LOG_LEVEL_DEFAULT);
		REQUIRE(config.log_file == CONFIG_LOG_FILE_DEFAULT);
		REQUIRE(config.profiling_enabled ==
			CONFIG_PROFILING_ENABLED_DEFAULT);
		REQUIRE(config.version_ignore == CONFIG_VERSION_IGNORE_DEFAULT);
	}
}

TEST_CASE("vaccel_config_release", "[core][config]")
{
	int ret;
	const char *plugins = "libvaccel-noop.so:libvaccel-exec.so";
	vaccel_log_level_t const log_level = VACCEL_LOG_DEBUG;
	const char *log_file = "vaccel_log";
	bool const profiling_enabled = true;
	bool const version_ignore = true;
	struct vaccel_config config = {
		.plugins = CONFIG_PLUGINS_DEFAULT,
		.log_level = CONFIG_LOG_LEVEL_DEFAULT,
		.log_file = CONFIG_LOG_FILE_DEFAULT,
		.profiling_enabled = CONFIG_PROFILING_ENABLED_DEFAULT,
		.version_ignore = CONFIG_VERSION_IGNORE_DEFAULT
	};

	ret = vaccel_config_init(&config, plugins, log_level, log_file,
				 profiling_enabled, version_ignore);
	REQUIRE(ret == VACCEL_OK);

	SECTION("invalid arguments")
	{
		ret = vaccel_config_release(nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(strcmp(config.plugins, plugins) == 0);
		REQUIRE(config.log_level == log_level);
		REQUIRE(strcmp(config.log_file, log_file) == 0);
		REQUIRE(config.profiling_enabled == profiling_enabled);
		REQUIRE(config.version_ignore == version_ignore);
	}

	ret = vaccel_config_release(&config);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(config.plugins == CONFIG_PLUGINS_DEFAULT);
	REQUIRE(config.log_level == CONFIG_LOG_LEVEL_DEFAULT);
	REQUIRE(config.log_file == CONFIG_LOG_FILE_DEFAULT);
	REQUIRE(config.profiling_enabled == CONFIG_PROFILING_ENABLED_DEFAULT);
	REQUIRE(config.version_ignore == CONFIG_VERSION_IGNORE_DEFAULT);
}

TEST_CASE("vaccel_config_new", "[core][config]")
{
	int ret;
	const char *plugins = "libvaccel-noop.so:libvaccel-exec.so";
	vaccel_log_level_t const log_level = VACCEL_LOG_DEBUG;
	const char *log_file = "vaccel_log";
	bool const profiling_enabled = true;
	bool const version_ignore = true;
	struct vaccel_config *config = nullptr;

	SECTION("success")
	{
		ret = vaccel_config_new(&config, plugins, log_level, log_file,
					profiling_enabled, version_ignore);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(config != nullptr);

		REQUIRE(vaccel_config_delete(config) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_config_new(nullptr, plugins, log_level, log_file,
					profiling_enabled, version_ignore);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(config == nullptr);
	}
}

TEST_CASE("vaccel_config_delete", "[core][config]")
{
	int ret;
	struct vaccel_config *config = nullptr;

	REQUIRE(vaccel_config_from_env(&config) == VACCEL_OK);

	SECTION("invalid arguments")
	{
		ret = vaccel_config_delete(nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	ret = vaccel_config_delete(config);
	REQUIRE(ret == VACCEL_OK);
}

TEST_CASE("config_create_from_existing_and_print", "[core][config]")
{
	int ret;
	const char *plugins = "libvaccel-noop.so:libvaccel-exec.so";
	vaccel_log_level_t const log_level = VACCEL_LOG_DEBUG;
	const char *log_file = "vaccel_log";
	bool const profiling_enabled = true;
	bool const version_ignore = true;
	struct vaccel_config config_src;
	struct vaccel_config *config;

	ret = vaccel_config_init(&config_src, plugins, log_level, log_file,
				 profiling_enabled, version_ignore);
	REQUIRE(ret == VACCEL_OK);

	REQUIRE(vaccel_config_from(&config, &config_src) == VACCEL_OK);

	vaccel_config_print_debug(config);

	REQUIRE(vaccel_config_release(&config_src) == VACCEL_OK);
	REQUIRE(vaccel_config_delete(config) == VACCEL_OK);
}
