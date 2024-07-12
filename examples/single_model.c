// SPDX-License-Identifier: Apache-2.0

#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../src/utils.h"
#include <vaccel.h>

int create_session(struct vaccel_session *sess)
{
	int ret = vaccel_sess_init(sess, 0);
	if (ret) {
		vaccel_error("Could not initialize session");
		return ret;
	}

	vaccel_info("New session: %u", sess->session_id);
	return VACCEL_OK;
}

int destroy_session(struct vaccel_session *sess)
{
	vaccel_sess_free(sess);
	return VACCEL_OK;
}

int create_from_path(const char *path)
{
	vaccel_info("Creating new model handle");
	struct vaccel_single_model *model = vaccel_single_model_new();
	if (!model) {
		vaccel_error("Could not create model");
		return VACCEL_ENOMEM;
	}

	vaccel_info("Setting path of the model");
	int ret = vaccel_single_model_set_path(model, path);
	if (ret) {
		vaccel_error("Could not load saved model");
		return ret;
	}

	vaccel_info("Registering model resource with vAccel");
	ret = vaccel_single_model_register(model);
	if (ret) {
		vaccel_error("Could not create model resource");
		return ret;
	}

	vaccel_id_t model_id = vaccel_single_model_get_id(model);
	vaccel_info("Registered new resource: %lld", model_id);

	struct vaccel_session sess;
	ret = create_session(&sess);
	if (ret)
		return ret;

	vaccel_info("Registering model %lld with session %u", model_id,
		    sess.session_id);

	ret = vaccel_sess_register(&sess, model->resource);
	if (ret) {
		vaccel_error("Could not register model to session");
		return ret;
	}

	vaccel_info("Unregistering model %lld from session %u", model_id,
		    sess.session_id);

	ret = vaccel_sess_unregister(&sess, model->resource);
	if (ret) {
		vaccel_error("Could not unregister model from session");
		return ret;
	}

	vaccel_info("Destroying model %lld", model_id);
	ret = vaccel_single_model_destroy(model);
	if (ret) {
		vaccel_error("Could not destroy model");
		return ret;
	}

	free(model);

	vaccel_info("Destroying session %u", sess.session_id);
	return destroy_session(&sess);
}

int create_from_in_mem(const char *path)
{
	vaccel_info("Creating new model handle");
	struct vaccel_single_model *model = vaccel_single_model_new();
	if (!model) {
		vaccel_error("Could not create model");
		return VACCEL_ENOMEM;
	}

	unsigned char *ptr = NULL;
	size_t len;
	int ret = read_file(path, (void **)&ptr, &len);
	if (ret) {
		vaccel_error("Could not read model from path");
		return VACCEL_ENOMEM;
	}

	ret = vaccel_single_model_set_file(model, NULL, ptr, len);
	if (ret) {
		vaccel_error("Could not set file for model");
		return ret;
	}

	vaccel_info("Registering model resource with vAccel");
	ret = vaccel_single_model_register(model);
	if (ret) {
		vaccel_error("Could not create model resource");
		return ret;
	}

	vaccel_id_t model_id = vaccel_single_model_get_id(model);
	vaccel_info("Registered new resource: %lld", model_id);

	struct vaccel_session sess;
	ret = create_session(&sess);
	if (ret)
		return ret;

	vaccel_info("Registering model %lld with session %u", model_id,
		    sess.session_id);

	ret = vaccel_sess_register(&sess, model->resource);
	if (ret) {
		vaccel_error("Could not register model to session");
		return ret;
	}

	vaccel_info("Unregistering model %lld from session %u", model_id,
		    sess.session_id);

	ret = vaccel_sess_unregister(&sess, model->resource);
	if (ret) {
		vaccel_error("Could not unregister model from session");
		return ret;
	}

	vaccel_info("Destroying model %lld", model_id);
	ret = vaccel_single_model_destroy(model);
	if (ret) {
		vaccel_error("Could not destroy model");
		return ret;
	}

	free(model);
	free(ptr);

	vaccel_info("Destroying session %u", sess.session_id);
	return destroy_session(&sess);
}

int main(int argc, char *argv[])
{
	if (argc != 2) {
		vaccel_info("usage: %s saved_model_path", argv[0]);
		return 0;
	}

	vaccel_info("Testing model handling from path");
	int ret = create_from_path(argv[1]);
	if (ret)
		return ret;

	vaccel_info("Testing model handling from in memory data");
	return create_from_in_mem(argv[1]);
}
