// SPDX-License-Identifier: Apache-2.0

#include "vaccel.h"
#include <cstdint>

static struct vaccel_plugin plugin;
static struct vaccel_plugin_info plugin_info;

static auto mock_virtio_session_init(struct vaccel_session *sess,
				     uint32_t flags) -> int
{
	sess->remote_id = 1;
	(void)flags;
	return VACCEL_OK;
}

static auto mock_virtio_session_update(struct vaccel_session *sess,
				       uint32_t flags) -> int
{
	(void)sess;
	(void)flags;
	return VACCEL_OK;
}

static auto mock_virtio_session_free(struct vaccel_session *sess) -> int
{
	(void)sess;
	return VACCEL_OK;
}

static auto mock_virtio_resource_register(struct vaccel_resource *res,
					  struct vaccel_session *sess) -> int
{
	res->remote_id = 1;
	(void)sess;
	return VACCEL_OK;
}

static auto mock_virtio_resource_unregister(struct vaccel_resource *res,
					    struct vaccel_session *sess) -> int
{
	(void)res;
	(void)sess;
	return VACCEL_OK;
}

auto mock_virtio_plugin_virtio() -> struct vaccel_plugin *
{
	plugin_info.name = "fake_virtio";
	plugin_info.session_init = mock_virtio_session_init;
	plugin_info.session_release = mock_virtio_session_free;
	plugin_info.session_update = mock_virtio_session_update;
	plugin_info.resource_register = mock_virtio_resource_register;
	plugin_info.resource_unregister = mock_virtio_resource_unregister;

	plugin.info = &plugin_info;

	return &plugin;
}
