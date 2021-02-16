#include "vaccel.h"
#include "plugin.h"
#include "log.h"
#include "id_pool.h"

#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#define MAX_VACCEL_SESSIONS 1024

struct {
	/* true if the sessions subsystem has been initialized */
	bool initialized;

	/* Id pool for sessions */
	id_pool_t *ids;
} sessions;

static uint32_t get_sess_id(void)
{
	return get_new_id(sessions.ids);
}

static void put_sess_id(uint32_t id)
{
	release_id(sessions.ids, id);
}

int sessions_bootstrap(void)
{
	sessions.ids = id_pool_new(MAX_VACCEL_SESSIONS);
	if (!sessions.ids)
		return VACCEL_ENOMEM;

	sessions.initialized = true;

	return VACCEL_OK;
}

int vaccel_sess_init(struct vaccel_session *sess, uint32_t flags)
{
	if (!sess)
		return VACCEL_EINVAL;

	if (!sessions.initialized)
		return VACCEL_ESESS;

	/* if we're using virtio as a plugin offload the session initialization
	 * to the host */
	struct vaccel_plugin *virtio = get_virtio_plugin();
	if (virtio)
		return virtio->info->sess_init(sess, flags);

	uint32_t sess_id = get_sess_id();
	if (!sess_id)
		return VACCEL_ESESS;

	sess->session_id = sess_id;
	list_init(&sess->resources);

	vaccel_debug("session:%u New session", sess_id);

	return VACCEL_OK;
}

int vaccel_sess_free(struct vaccel_session *sess)
{
	if (!sess)
		return VACCEL_EINVAL;

	if (!sessions.initialized)
		return VACCEL_ESESS;

	/* if we're using virtio as a plugin offload the session cleanup to the
	 * host */
	struct vaccel_plugin *virtio = get_virtio_plugin();
	if (virtio)
		return virtio->info->sess_free(sess);

	put_sess_id(sess->session_id);

	vaccel_debug("session:%u Free session", sess->session_id);

	return VACCEL_OK;
}
