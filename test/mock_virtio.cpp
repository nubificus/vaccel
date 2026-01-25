// SPDX-License-Identifier: Apache-2.0

#include "vaccel.h"
#include <cstdint>
#include <string>

static struct vaccel_plugin plugin;
static struct vaccel_plugin_info plugin_info;
static std::string remote_name;

static auto mock_virtio_init() -> int
{
	return VACCEL_OK;
}

static auto mock_virtio_fini() -> int
{
	return VACCEL_OK;
}

static auto mock_virtio_session_init(struct vaccel_session *sess,
				     uint32_t flags) -> int
{
	sess->remote_id = 1;
	sess->hint = flags;
	return VACCEL_OK;
}

static auto mock_virtio_session_init_with_plugin(struct vaccel_session *sess,
						 const char *plugin_spec) -> int
{
	sess->remote_id = 1;
	remote_name = plugin_spec;
	return VACCEL_OK;
}

static auto mock_virtio_session_update(struct vaccel_session *sess,
				       uint32_t flags) -> int
{
	sess->hint = flags;
	return VACCEL_OK;
}

static auto mock_virtio_session_release(struct vaccel_session *sess) -> int
{
	sess->remote_id = 0;
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
	res->remote_id = 0;
	(void)sess;
	return VACCEL_OK;
}

auto mock_virtio_plugin_virtio() -> struct vaccel_plugin *
{
	plugin_info.name = "fake_virtio";
	plugin_info.version = VACCEL_VERSION;
	plugin_info.vaccel_version = VACCEL_VERSION;
	plugin_info.is_virtio = true;
	plugin_info.type = VACCEL_PLUGIN_DEBUG;
	plugin_info.init = mock_virtio_init;
	plugin_info.fini = mock_virtio_fini;
	plugin_info.session_init = mock_virtio_session_init;
	plugin_info.session_init_with_plugin =
		mock_virtio_session_init_with_plugin;
	plugin_info.session_release = mock_virtio_session_release;
	plugin_info.session_update = mock_virtio_session_update;
	plugin_info.resource_register = mock_virtio_resource_register;
	plugin_info.resource_unregister = mock_virtio_resource_unregister;

	plugin.dl_handle = nullptr;
	list_init(&plugin.entry);
	plugin.info = &plugin_info;
	for (auto &op : plugin.ops)
		op = nullptr;

	remote_name.clear();
	return &plugin;
}

auto mock_virtio_plugin_remote_name() -> const char *
{
	return remote_name.c_str();
}
