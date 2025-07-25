// SPDX-License-Identifier: Apache-2.0

/*
 * The code below performs unit testing to sessions.
 *
 * 1) vaccel_session_init()
 * 2) vaccel_session_release()
 * 3) vaccel_session_new()
 * 4) vaccel_session_delete()
 * 5) vaccel_session_update()
 * 6) session_register_resource()
 * 7) session_unregister_resource()
 * 8) vaccel_session_has_resource()
 * 9) vaccel_session_resource_by_type()
 * 10) vaccel_session_resource_by_id()
 * 11) vaccel_session_resources_by_type()
 *
 */

#include "vaccel.h"
#include <catch.hpp>
#include <cerrno>
#include <cstdlib>
#include <fff.h>
#include <mock_virtio.hpp>

DEFINE_FFF_GLOBALS;

enum { MAX_VACCEL_SESSIONS = 1024 };

TEST_CASE("vaccel_session_init", "[core][session]")
{
	int ret;

	struct vaccel_session sess;
	ret = vaccel_session_init(&sess, 1);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(sess.id > 0);
	REQUIRE(sess.hint == 1);
	REQUIRE(sess.resources);
	REQUIRE(sess.priv == nullptr);

	SECTION("invalid arguments")
	{
		ret = vaccel_session_init(nullptr, 1);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);
}

TEST_CASE("vaccel_session_release", "[core][session]")
{
	int ret;

	struct vaccel_session sess;
	REQUIRE(vaccel_session_init(&sess, 1) == VACCEL_OK);

	SECTION("invalid arguments")
	{
		ret = vaccel_session_release(nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(sess.id > 0);
		REQUIRE(sess.hint == 1);
		REQUIRE(sess.resources);
		REQUIRE(sess.priv == nullptr);
	}

	ret = vaccel_session_release(&sess);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(sess.id == 0);
	REQUIRE(sess.hint == 1);
	REQUIRE(sess.resources == nullptr);
	REQUIRE(sess.priv == nullptr);
}

TEST_CASE("vaccel_session_new", "[core][session]")
{
	int ret;

	struct vaccel_session *sess;
	ret = vaccel_session_new(&sess, 1);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(sess->id);
	REQUIRE(sess->hint == 1);
	REQUIRE(sess->resources);
	REQUIRE(sess->priv == nullptr);

	SECTION("invalid arguments")
	{
		ret = vaccel_session_new(nullptr, 1);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	REQUIRE(vaccel_session_delete(sess) == VACCEL_OK);
}

TEST_CASE("vaccel_session_delete", "[core][session]")
{
	int ret;

	struct vaccel_session *sess;
	REQUIRE(vaccel_session_new(&sess, 1) == VACCEL_OK);

	SECTION("invalid arguments")
	{
		ret = vaccel_session_delete(nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(sess->id);
		REQUIRE(sess->hint == 1);
		REQUIRE(sess->resources);
		REQUIRE(sess->priv == nullptr);
	}

	ret = vaccel_session_delete(sess);
	REQUIRE(ret == VACCEL_OK);
}

TEST_CASE("vaccel_session_update", "[core][session]")
{
	int ret;

	struct vaccel_session sess;
	REQUIRE(vaccel_session_init(&sess, 1) == VACCEL_OK);

	ret = vaccel_session_update(&sess, 2);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(sess.hint == 2);

	SECTION("invalid arguments")
	{
		ret = vaccel_session_update(nullptr, 1);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);
}

TEST_CASE("session_register_resource", "[core][session]")
{
	int ret;

	struct vaccel_session sess;
	REQUIRE(vaccel_session_init(&sess, 1) == VACCEL_OK);

	struct vaccel_resource res;
	res.type = VACCEL_RESOURCE_LIB;
	res.id = 1;

	SECTION("invalid arguments")
	{
		res.type = VACCEL_RESOURCE_MAX;
		ret = session_register_resource(&sess, &res);
		REQUIRE(ret == VACCEL_EINVAL);
		res.type = VACCEL_RESOURCE_LIB;

		ret = session_register_resource(nullptr, nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
		ret = session_register_resource(nullptr, &res);
		REQUIRE(ret == VACCEL_EINVAL);
		ret = session_register_resource(&sess, nullptr);
		REQUIRE(ret == VACCEL_EINVAL);

		REQUIRE(list_empty(&sess.resources->registered[res.type]));
	}

	ret = session_register_resource(&sess, &res);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE_FALSE(list_empty(&sess.resources->registered[res.type]));

	SECTION("already registered")
	{
		ret = session_register_resource(&sess, &res);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	REQUIRE(session_unregister_resource(&sess, &res) == VACCEL_OK);
	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);
}

TEST_CASE("session_unregister_resource", "[core][session]")
{
	int ret;

	struct vaccel_session sess;
	REQUIRE(vaccel_session_init(&sess, 1) == VACCEL_OK);

	struct vaccel_resource res;
	res.type = VACCEL_RESOURCE_LIB;
	res.id = 1;

	REQUIRE(session_register_resource(&sess, &res) == VACCEL_OK);

	SECTION("invalid arguments")
	{
		res.type = VACCEL_RESOURCE_MAX;
		ret = session_unregister_resource(&sess, &res);
		REQUIRE(ret == VACCEL_EINVAL);
		res.type = VACCEL_RESOURCE_LIB;

		ret = session_unregister_resource(nullptr, &res);
		REQUIRE(ret == VACCEL_EINVAL);
		ret = session_unregister_resource(&sess, nullptr);
		REQUIRE(ret == VACCEL_EINVAL);

		REQUIRE_FALSE(
			list_empty(&sess.resources->registered[res.type]));
	}

	ret = session_unregister_resource(&sess, &res);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(list_empty(&sess.resources->registered[res.type]));

	SECTION("already unregistered")
	{
		ret = session_unregister_resource(&sess, &res);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);
}

TEST_CASE("vaccel_session_has_resource", "[core][session]")
{
	struct vaccel_session sess;
	REQUIRE(vaccel_session_init(&sess, 1) == VACCEL_OK);

	struct vaccel_resource res;
	res.type = VACCEL_RESOURCE_LIB;
	res.id = 1;

	REQUIRE(session_register_resource(&sess, &res) == VACCEL_OK);

	REQUIRE(vaccel_session_has_resource(&sess, &res));

	SECTION("invalid arguments")
	{
		REQUIRE_FALSE(vaccel_session_has_resource(nullptr, &res));
		REQUIRE_FALSE(vaccel_session_has_resource(&sess, nullptr));
	}

	REQUIRE(session_unregister_resource(&sess, &res) == VACCEL_OK);

	REQUIRE_FALSE(vaccel_session_has_resource(&sess, &res));

	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);
}

TEST_CASE("session_ops", "[core][session]")
{
	struct vaccel_session sess;
	REQUIRE(vaccel_session_init(&sess, 1) == VACCEL_OK);

	struct vaccel_session *alloc_sess;
	REQUIRE(vaccel_session_new(&alloc_sess, 1) == VACCEL_OK);

	struct vaccel_resource res;
	res.type = VACCEL_RESOURCE_LIB;
	res.id = 1;

	REQUIRE(vaccel_session_update(&sess, 2) == VACCEL_OK);
	REQUIRE(sess.hint == 2);
	REQUIRE(vaccel_session_update(alloc_sess, 2) == VACCEL_OK);
	REQUIRE(alloc_sess->hint == 2);

	REQUIRE(session_register_resource(&sess, &res) == VACCEL_OK);
	REQUIRE(session_register_resource(alloc_sess, &res) == VACCEL_OK);

	REQUIRE(vaccel_session_has_resource(&sess, &res));
	REQUIRE(vaccel_session_has_resource(alloc_sess, &res));

	REQUIRE(vaccel_session_update(&sess, 3) == VACCEL_OK);
	REQUIRE(sess.hint == 3);
	REQUIRE(vaccel_session_update(alloc_sess, 3) == VACCEL_OK);
	REQUIRE(alloc_sess->hint == 3);

	REQUIRE(session_unregister_resource(&sess, &res) == VACCEL_OK);
	REQUIRE(session_unregister_resource(alloc_sess, &res) == VACCEL_OK);

	REQUIRE_FALSE(vaccel_session_has_resource(&sess, &res));
	REQUIRE_FALSE(vaccel_session_has_resource(alloc_sess, &res));

	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);
	REQUIRE(vaccel_session_delete(alloc_sess) == VACCEL_OK);
}

TEST_CASE("session_virtio", "[core][session]")
{
	struct vaccel_session sess;

	SECTION("not_registered")
	{
		REQUIRE(vaccel_session_init(&sess, 1 | VACCEL_PLUGIN_REMOTE) ==
			VACCEL_ENOTSUP);
	}

	auto *virtio_plugin = mock_virtio_plugin_virtio();
	REQUIRE(plugin_register(virtio_plugin) == VACCEL_OK);

	REQUIRE(vaccel_session_init(&sess,
				    VACCEL_PLUGIN_CPU | VACCEL_PLUGIN_REMOTE) ==
		VACCEL_OK);

	struct vaccel_session *alloc_sess;
	REQUIRE(vaccel_session_new(&alloc_sess,
				   VACCEL_PLUGIN_CPU | VACCEL_PLUGIN_REMOTE) ==
		VACCEL_OK);

	struct vaccel_resource res;
	res.type = VACCEL_RESOURCE_LIB;
	res.id = 1;

	REQUIRE(vaccel_session_update(&sess, VACCEL_PLUGIN_GPU) == VACCEL_OK);
	REQUIRE(sess.hint == VACCEL_PLUGIN_GPU);
	REQUIRE(vaccel_session_update(alloc_sess, VACCEL_PLUGIN_GPU) ==
		VACCEL_OK);
	REQUIRE(alloc_sess->hint == VACCEL_PLUGIN_GPU);

	REQUIRE(session_register_resource(&sess, &res) == VACCEL_OK);
	REQUIRE(session_register_resource(alloc_sess, &res) == VACCEL_OK);

	REQUIRE(vaccel_session_has_resource(&sess, &res));
	REQUIRE(vaccel_session_has_resource(alloc_sess, &res));

	REQUIRE(vaccel_session_update(&sess, VACCEL_PLUGIN_FPGA) == VACCEL_OK);
	REQUIRE(sess.hint == VACCEL_PLUGIN_FPGA);
	REQUIRE(vaccel_session_update(alloc_sess, VACCEL_PLUGIN_FPGA) ==
		VACCEL_OK);
	REQUIRE(alloc_sess->hint == VACCEL_PLUGIN_FPGA);

	REQUIRE(session_unregister_resource(&sess, &res) == VACCEL_OK);
	REQUIRE(session_unregister_resource(alloc_sess, &res) == VACCEL_OK);

	REQUIRE_FALSE(vaccel_session_has_resource(&sess, &res));
	REQUIRE_FALSE(vaccel_session_has_resource(alloc_sess, &res));

	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);
	REQUIRE(vaccel_session_delete(alloc_sess) == VACCEL_OK);

	REQUIRE(plugin_unregister(virtio_plugin) == VACCEL_OK);
}

TEST_CASE("vaccel_session_resource_by_type", "[core][session]")
{
	int ret;
	struct vaccel_resource *res_ptr;
	struct vaccel_session sess;

	SECTION("invalid arguments")
	{
		ret = vaccel_session_resource_by_type(nullptr, &res_ptr,
						      VACCEL_RESOURCE_LIB);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_session_resource_by_type(&sess, nullptr,
						      VACCEL_RESOURCE_LIB);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_session_resource_by_type(&sess, &res_ptr,
						      VACCEL_RESOURCE_MAX);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);

	SECTION("no resource")
	{
		ret = vaccel_session_resource_by_type(&sess, &res_ptr,
						      VACCEL_RESOURCE_LIB);
		REQUIRE(ret == VACCEL_ENOENT);

		ret = vaccel_session_resource_by_type(&sess, &res_ptr,
						      VACCEL_RESOURCE_MODEL);
		REQUIRE(ret == VACCEL_ENOENT);

		ret = vaccel_session_resource_by_type(&sess, &res_ptr,
						      VACCEL_RESOURCE_DATA);
		REQUIRE(ret == VACCEL_ENOENT);
	}

	struct vaccel_resource res;
	res.type = VACCEL_RESOURCE_LIB;
	res.id = 1;

	SECTION("resource not registered")
	{
		ret = vaccel_session_resource_by_type(&sess, &res_ptr,
						      VACCEL_RESOURCE_LIB);
		REQUIRE(ret == VACCEL_ENOENT);
	}

	REQUIRE(session_register_resource(&sess, &res) == VACCEL_OK);

	ret = vaccel_session_resource_by_type(&sess, &res_ptr,
					      VACCEL_RESOURCE_LIB);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(res_ptr == &res);

	SECTION("invalid resource type")
	{
		ret = vaccel_session_resource_by_type(&sess, &res_ptr,
						      VACCEL_RESOURCE_MODEL);
		REQUIRE(ret == VACCEL_ENOENT);

		ret = vaccel_session_resource_by_type(&sess, &res_ptr,
						      VACCEL_RESOURCE_DATA);
		REQUIRE(ret == VACCEL_ENOENT);
	}

	REQUIRE(session_unregister_resource(&sess, &res) == VACCEL_OK);

	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);
}

TEST_CASE("vaccel_session_resource_by_id", "[core][session]")
{
	int ret;
	struct vaccel_resource *res_ptr;
	struct vaccel_session sess;

	SECTION("invalid arguments")
	{
		ret = vaccel_session_resource_by_id(nullptr, &res_ptr, 1);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_session_resource_by_id(&sess, nullptr, 1);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_session_resource_by_id(&sess, &res_ptr, 0);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);

	SECTION("no resource")
	{
		ret = vaccel_session_resource_by_id(&sess, &res_ptr, 1);
		REQUIRE(ret == VACCEL_ENOENT);
	}

	struct vaccel_resource res;
	res.type = VACCEL_RESOURCE_LIB;
	res.id = 1;

	SECTION("resource not registered")
	{
		ret = vaccel_session_resource_by_id(&sess, &res_ptr, res.id);
		REQUIRE(ret == VACCEL_ENOENT);
	}

	REQUIRE(session_register_resource(&sess, &res) == VACCEL_OK);

	ret = vaccel_session_resource_by_id(&sess, &res_ptr, res.id);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(res_ptr == &res);

	SECTION("invalid resource id")
	{
		ret = vaccel_session_resource_by_id(&sess, &res_ptr,
						    res.id + 1);
		REQUIRE(ret == VACCEL_ENOENT);
	}

	REQUIRE(session_unregister_resource(&sess, &res) == VACCEL_OK);

	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);
}

TEST_CASE("vaccel_session_resources_by_type", "[core][session]")
{
	int ret;
	struct vaccel_resource **res_ptr;
	struct vaccel_session sess;
	size_t nr_found;

	SECTION("invalid arguments")
	{
		ret = vaccel_session_resources_by_type(
			nullptr, &res_ptr, &nr_found, VACCEL_RESOURCE_LIB);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_session_resources_by_type(
			&sess, nullptr, &nr_found, VACCEL_RESOURCE_LIB);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_session_resources_by_type(&sess, &res_ptr, nullptr,
						       VACCEL_RESOURCE_LIB);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_session_resources_by_type(
			&sess, &res_ptr, &nr_found, VACCEL_RESOURCE_MAX);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	REQUIRE(vaccel_session_init(&sess, 0) == VACCEL_OK);

	SECTION("no resource")
	{
		ret = vaccel_session_resources_by_type(
			&sess, &res_ptr, &nr_found, VACCEL_RESOURCE_LIB);
		REQUIRE(ret == VACCEL_ENOENT);
		REQUIRE(nr_found == 0);

		ret = vaccel_session_resources_by_type(
			&sess, &res_ptr, &nr_found, VACCEL_RESOURCE_MODEL);
		REQUIRE(ret == VACCEL_ENOENT);
		REQUIRE(nr_found == 0);

		ret = vaccel_session_resources_by_type(
			&sess, &res_ptr, &nr_found, VACCEL_RESOURCE_DATA);
		REQUIRE(ret == VACCEL_ENOENT);
		REQUIRE(nr_found == 0);
	}

	struct vaccel_resource res;
	res.type = VACCEL_RESOURCE_LIB;
	res.id = 1;

	SECTION("resource not registered")
	{
		ret = vaccel_session_resources_by_type(
			&sess, &res_ptr, &nr_found, VACCEL_RESOURCE_LIB);
		REQUIRE(ret == VACCEL_ENOENT);
		REQUIRE(nr_found == 0);
	}

	REQUIRE(session_register_resource(&sess, &res) == VACCEL_OK);

	ret = vaccel_session_resources_by_type(&sess, &res_ptr, &nr_found,
					       VACCEL_RESOURCE_LIB);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(nr_found == 1);
	REQUIRE(res_ptr[0] == &res);

	free(res_ptr);

	/* Success with more than 1 resource */
	struct vaccel_resource res2;
	res2.type = VACCEL_RESOURCE_LIB;
	res2.id = 1;

	REQUIRE(session_register_resource(&sess, &res2) == VACCEL_OK);

	ret = vaccel_session_resources_by_type(&sess, &res_ptr, &nr_found,
					       VACCEL_RESOURCE_LIB);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(nr_found == 2);
	REQUIRE(res_ptr[0] == &res);
	REQUIRE(res_ptr[1] == &res2);

	free(res_ptr);

	REQUIRE(session_unregister_resource(&sess, &res2) == VACCEL_OK);

	SECTION("invalid resource type")
	{
		ret = vaccel_session_resources_by_type(
			&sess, &res_ptr, &nr_found, VACCEL_RESOURCE_MODEL);
		REQUIRE(ret == VACCEL_ENOENT);

		ret = vaccel_session_resources_by_type(
			&sess, &res_ptr, &nr_found, VACCEL_RESOURCE_DATA);
		REQUIRE(ret == VACCEL_ENOENT);
	}

	REQUIRE(session_unregister_resource(&sess, &res) == VACCEL_OK);

	REQUIRE(vaccel_session_release(&sess) == VACCEL_OK);
}
