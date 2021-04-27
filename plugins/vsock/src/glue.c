#include <vaccel.h>
#include <plugin.h>

#include <string.h>

struct VsockClient;

/* Handle session connection */
extern struct VsockClient *create_client();
extern void destroy_client(struct VsockClient *);

/* Session handling */
extern int sess_init(const struct VsockClient *, uint32_t);
extern int sess_free(const struct VsockClient *, uint32_t);

/* Image operations */
extern int image_classify(
        const struct VsockClient *,
        uint32_t sess_id,
        const unsigned char *img,
	size_t img_len,
        unsigned char *tags,
        size_t tags_len
);

/* Resource handling */
extern vaccel_id_t create_resource(
        const struct VsockClient *client,
        vaccel_resource_t type,
        void *data
);
extern int destroy_resource(const struct VsockClient *client, vaccel_id_t id);
extern int register_resource(
		const struct VsockClient *client,
		vaccel_id_t res_id,
		uint32_t sess_id
);
extern int unregister_resource(
		const struct VsockClient *client,
		vaccel_id_t res_id,
		uint32_t sess_id
);

static int vsock_sess_init(struct vaccel_session *sess, uint32_t flags)
{
    if (!sess)
        return VACCEL_EINVAL;

    struct VsockClient *client = create_client();
    if (!client)
        return VACCEL_ESESS;

    vaccel_debug("[vsock] Initializing session");

    sess->priv = (void *)client;
    int ret = sess_init(client, flags);

    if (ret <= 0) {
        vaccel_error("[vsock] sess_init failed. Host returned %d", ret);
    }

    vaccel_debug("[vsock] New session %d", ret);
    sess->session_id = ret;

    return VACCEL_OK;
}

static int vsock_sess_free(struct vaccel_session *sess)
{
    if (!sess)
        return VACCEL_EINVAL;

    vaccel_debug("[vsock] Destroying session %u", sess->session_id);

    struct VsockClient *client = sess->priv;
    if (!client)
        return VACCEL_ESESS;

    int ret = sess_free(client, sess->session_id);

    vaccel_debug("[vsock] Destroying vsock client");

    /* Destroy the client object here regardless of whether the
     * call to destroy the session succeeded */
    destroy_client(client);
    sess->priv = NULL;

    return ret;
}

static int vsock_img_classify(struct vaccel_session *session, void *img,
        char *out_text, char *out_imgname, size_t len_img,
        size_t len_out_text, size_t len_out_imgname)
{
    (void)out_imgname;
    (void)len_out_imgname;
    if (!session)
        return VACCEL_EINVAL;

    struct VsockClient *client = (struct VsockClient *)session->priv;
    if (!client)
        return VACCEL_EINVAL;

    return image_classify(client, session->session_id, img, len_img, (unsigned char *)out_text,
            len_out_text);
}

static int vsock_resource_new(vaccel_resource_t type, void *data, vaccel_id_t *id)
{
    if (!data || !id)
        return VACCEL_EINVAL;

    vaccel_debug("[vsock] Creating new resource on host");

    struct VsockClient *client = create_client();
    if (!client)
        return VACCEL_ESESS;

    vaccel_id_t ret = create_resource(client, type, data);    
    destroy_client(client);

    vaccel_debug("[vsock] Host returned %lu", ret);

    if (ret <= 0)
        return -ret;

    *id = ret;
    return VACCEL_OK;
}

static int vsock_resource_destroy(vaccel_id_t id)
{
    struct VsockClient *client = create_client();
    if (!client)
        return VACCEL_ESESS;

    int ret = destroy_resource(client, id); 
    destroy_client(client);

    return ret;
}

static int vsock_sess_register(uint32_t sess_id, vaccel_id_t res_id)
{
    struct VsockClient *client = create_client();
    if (!client)
        return VACCEL_ESESS;

    int ret = register_resource(client, res_id, sess_id); 
    destroy_client(client);

    return ret;
}

static int vsock_sess_unregister(uint32_t sess_id, vaccel_id_t res_id)
{
    struct VsockClient *client = create_client();
    if (!client)
        return VACCEL_ESESS;

    int ret = unregister_resource(client, res_id, sess_id); 
    destroy_client(client);

    return ret;
}

struct vaccel_op ops[] = {
    VACCEL_OP_INIT(ops[0], VACCEL_IMG_CLASS, vsock_img_classify),
};

int vsock_init(void)
{
    int ret = register_plugin_functions(ops, sizeof(ops) / sizeof(ops[0]));
    if (!ret)
        return ret;

    return VACCEL_OK;
}

int vsock_fini(void)
{
    return VACCEL_OK;
}

VACCEL_MODULE(
        .name = "vsock",
        .version = "0.1",
        .init = vsock_init,
        .fini = vsock_fini,
        .is_virtio = true,
        .sess_init = vsock_sess_init,
        .sess_free = vsock_sess_free,
        .sess_register = vsock_sess_register,
        .sess_unregister = vsock_sess_unregister,
        .resource_new = vsock_resource_new,
        .resource_destroy = vsock_resource_destroy,
        )
