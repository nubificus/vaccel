#include "vaccel.h"
#include "backend.h"
#include "log.h"

#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#define MAX_VACCEL_SESSIONS 1024

struct {
	/* true if the sessions subsystem has been initialized */
	bool initialized;

	/* Available session ids */
	uint32_t ids[MAX_VACCEL_SESSIONS];

	/* Next free session id position */
	size_t next_free;

	/* Lock to access the session ids array */
	pthread_spinlock_t lock;
} sessions;

static uint32_t get_sess_id(void)
{
	uint32_t id;

	pthread_spin_lock(&sessions.lock);
	if (sessions.next_free == MAX_VACCEL_SESSIONS)
		id = 0;
	else
		id = sessions.ids[sessions.next_free++];
	pthread_spin_unlock(&sessions.lock);

	return id;
}

static void put_sess_id(uint32_t id)
{
	pthread_spin_lock(&sessions.lock);
	sessions.ids[--sessions.next_free] = id;
	pthread_spin_unlock(&sessions.lock);
}

int sessions_bootstrap(void)
{
	int ret = pthread_spin_init(&sessions.lock, PTHREAD_PROCESS_PRIVATE);
	if (ret) {
		sessions.initialized = false;
		return VACCEL_ESESS;
	}

	for (size_t i = 0; i < MAX_VACCEL_SESSIONS; ++i)
		sessions.ids[i] = i + 1;

	sessions.initialized = true;
	sessions.next_free = 0;

	return VACCEL_OK;
}

int vaccel_sess_init(struct vaccel_session *sess, uint32_t flags)
{
	if (!sess)
		return VACCEL_EINVAL;

	if (!sessions.initialized)
		return VACCEL_ESESS;

	/* if we're using virtio as a backend offload the session initialization
	 * to the host */
	struct vaccel_backend *virtio = get_virtio_backend();
	if (virtio)
		return virtio->vaccel_sess_init(sess, flags);

	uint32_t sess_id = get_sess_id();
	if (!sess_id)
		return VACCEL_ESESS;

	sess->session_id = sess_id;

	vaccel_debug("session:%u New session", sess_id);

	return VACCEL_OK;
}

int vaccel_sess_free(struct vaccel_session *sess)
{
	if (!sess)
		return VACCEL_EINVAL;

	if (!sessions.initialized)
		return VACCEL_ESESS;

	/* if we're using virtio as a backend offload the session cleanup to the
	 * host */
	struct vaccel_backend *virtio = get_virtio_backend();
	if (virtio)
		return virtio->vaccel_sess_free(sess);

	put_sess_id(sess->session_id);

	vaccel_debug("session:%u Free session", sess->session_id);

	return VACCEL_OK;
}
