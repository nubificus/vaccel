// SPDX-License-Identifier: Apache-2.0

/*
 * The code below performs unit testing for bootstrap/cleanup of vAccel and
 * components
 *
 * 1) vaccel_bootstrap()
 * 2) vaccel_bootstrap_with_config()
 * 3) vaccel_cleanup()
 * 4) vaccel_is_initialized()
 * 5) vaccel_rundir()
 * 6) vaccel_config()
 * 7) vaccel_sessions_bootstrap()
 * 8) vaccel_sessions_cleanup()
 * 9) vaccel_resources_bootstrap()
 * 10) vaccel_resources_cleanup()
 * 11) vaccel_plugins_bootstrap()
 * 12) vaccel_plugins_cleanup()
 *
 */

#include "utils.hpp"
#include "vaccel.h"
#include <catch.hpp>
#include <cstdio>
#include <cstdlib>
#include <cstring>

TEST_CASE("bootstrap_and_cleanup", "[core]")
{
	int ret = vaccel_bootstrap();
	REQUIRE(ret == VACCEL_OK);

	/* Verify is initialized */
	REQUIRE(vaccel_is_initialized());

	/* Verify config has been set */
	struct vaccel_config config_env;
	REQUIRE(vaccel_config_init_from_env(&config_env) == VACCEL_OK);

	const struct vaccel_config *config = vaccel_config();
	REQUIRE(config != nullptr);
	REQUIRE((config->plugins == nullptr ||
		 strcmp(config->plugins, config_env.plugins) == 0));
	REQUIRE(config->log_level == config_env.log_level);
	REQUIRE((config->log_file == nullptr ||
		 strcmp(config->log_file, config_env.log_file) == 0));
	REQUIRE(config->profiling_enabled == config_env.profiling_enabled);
	REQUIRE(config->version_ignore == config_env.version_ignore);

	/* Verify rundir has been set */
	const char *rundir = vaccel_rundir();
	REQUIRE(rundir != nullptr);
	REQUIRE(strstr(rundir, "vaccel") != nullptr);

	REQUIRE(vaccel_config_release(&config_env) == VACCEL_OK);

	ret = vaccel_cleanup();
	REQUIRE(ret == VACCEL_OK);
}

TEST_CASE("bootstrap_with_config_and_cleanup", "[core]")
{
	char *plugins = abs_path(BUILD_ROOT, "plugins/noop/libvaccel-noop.so");
	vaccel_log_level_t const log_level = VACCEL_LOG_DEBUG;
	const char *log_file = "vaccel_log";
	bool const profiling_enabled = true;
	bool const version_ignore = true;
	struct vaccel_config config_test;

	int ret = vaccel_config_init(&config_test, plugins, log_level, log_file,
				     profiling_enabled, version_ignore);
	REQUIRE(ret == VACCEL_OK);

	SECTION("invalid arguments")
	{
		ret = vaccel_bootstrap_with_config(nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	ret = vaccel_bootstrap_with_config(&config_test);
	REQUIRE(ret == VACCEL_OK);

	/* Verify is initialized */
	REQUIRE(vaccel_is_initialized());

	/* Verify config has been set */
	const struct vaccel_config *config = vaccel_config();
	REQUIRE(config != nullptr);
	REQUIRE(strcmp(config->plugins, config_test.plugins) == 0);
	REQUIRE(config->log_level == config_test.log_level);
	REQUIRE(strcmp(config->log_file, config_test.log_file) == 0);
	REQUIRE(config->profiling_enabled == config_test.profiling_enabled);
	REQUIRE(config->version_ignore == config_test.version_ignore);

	/* Verify rundir has been set */
	const char *rundir = vaccel_rundir();
	REQUIRE(rundir != nullptr);
	REQUIRE(strstr(rundir, "vaccel") != nullptr);

	ret = vaccel_cleanup();
	REQUIRE(ret == VACCEL_OK);

	REQUIRE(vaccel_config_release(&config_test) == VACCEL_OK);
	free(plugins);
}

TEST_CASE("bootstrap_initialized_and_cleanup", "[core]")
{
	char *plugins = abs_path(BUILD_ROOT, "plugins/noop/libvaccel-noop.so");
	vaccel_log_level_t const log_level = VACCEL_LOG_DEBUG;
	const char *log_file = "vaccel_log";
	bool const profiling_enabled = true;
	bool const version_ignore = true;
	struct vaccel_config config_test;

	/* Initialize from env */
	int ret = vaccel_bootstrap();
	REQUIRE(ret == VACCEL_OK);

	ret = vaccel_config_init(&config_test, plugins, log_level, log_file,
				 profiling_enabled, version_ignore);
	REQUIRE(ret == VACCEL_OK);

	/* Initialize again with different config */
	ret = vaccel_bootstrap_with_config(&config_test);
	REQUIRE(ret == VACCEL_OK);

	/* Verify is initialized */
	REQUIRE(vaccel_is_initialized());

	/* Verify config has been set */
	const struct vaccel_config *config = vaccel_config();
	REQUIRE(config != nullptr);
	REQUIRE(strcmp(config->plugins, config_test.plugins) == 0);
	REQUIRE(config->log_level == config_test.log_level);
	REQUIRE(strcmp(config->log_file, config_test.log_file) == 0);
	REQUIRE(config->profiling_enabled == config_test.profiling_enabled);
	REQUIRE(config->version_ignore == config_test.version_ignore);

	/* Verify rundir has been set */
	const char *rundir = vaccel_rundir();
	REQUIRE(rundir != nullptr);
	REQUIRE(strstr(rundir, "vaccel") != nullptr);

	ret = vaccel_cleanup();
	REQUIRE(ret == VACCEL_OK);

	REQUIRE(vaccel_config_release(&config_test) == VACCEL_OK);
	free(plugins);
}

TEST_CASE("sessions_bootstrap_and_cleanup", "[core]")
{
	int ret;
	ret = sessions_bootstrap();
	REQUIRE(ret == VACCEL_OK);

	ret = sessions_cleanup();
	REQUIRE(ret == VACCEL_OK);
}

TEST_CASE("resources_bootstrap_and_cleanup", "[core]")
{
	int ret;
	ret = resources_bootstrap();
	REQUIRE(ret == VACCEL_OK);

	ret = resources_cleanup();
	REQUIRE(ret == VACCEL_OK);
}

TEST_CASE("plugins_bootstrap_and_cleanup", "[core]")
{
	int ret;
	ret = plugins_bootstrap();
	REQUIRE(ret == VACCEL_OK);

	ret = plugins_cleanup();
	REQUIRE(ret == VACCEL_OK);
}
