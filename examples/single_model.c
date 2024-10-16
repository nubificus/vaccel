// SPDX-License-Identifier: Apache-2.0

#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../src/utils.h"
#include <vaccel.h>

int create_session(struct vaccel_session *sess)
{
	int ret = vaccel_session_init(sess, 0);
	if (ret) {
		vaccel_error("Could not initialize session");
		return ret;
	}

	vaccel_info("New session: %u", sess->session_id);
	return VACCEL_OK;
}

int destroy_session(struct vaccel_session *sess)
{
	vaccel_session_free(sess);
	return VACCEL_OK;
}

int create_from_path(char *path)
{
	vaccel_info("Creating new model handle");
	struct vaccel_resource model;
	int ret = vaccel_resource_new(&model, path, VACCEL_FILE_DATA);
	if (ret)
		return ret;

	struct vaccel_session sess;
	ret = create_session(&sess);
	if (ret)
		return ret;

	vaccel_info("Registering model %lld with session %u", model.id,
		    sess.session_id);

	ret = vaccel_resource_register(&sess, &model);
	if (ret) {
		vaccel_error("Could not register model to session");
		return ret;
	}

	vaccel_info("Unregistering model %lld from session %u", model.id,
		    sess.session_id);

	ret = vaccel_resource_unregister(&sess, &model);
	if (ret) {
		vaccel_error("Could not unregister model from session");
		return ret;
	}

	vaccel_info("Destroying model %lld", model.id);
	ret = vaccel_resource_destroy(&model);
	if (ret) {
		vaccel_error("Could not destroy model resource");
		return ret;
	}

	vaccel_info("Destroying session %u", sess.session_id);
	return destroy_session(&sess);
}

int create_from_in_mem(const char *path)
{
	vaccel_info("Creating new model handle");
	struct vaccel_resource model;

	unsigned char *ptr = NULL;
	size_t len;
	int ret = read_file(path, (void **)&ptr, &len);
	if (ret) {
		vaccel_error("Could not read model from path");
		return VACCEL_ENOMEM;
	}

	ret = vaccel_resource_new_from_buf(&model, ptr, len, VACCEL_FILE_DATA);
	if (ret) {
		vaccel_error("Could not set file for model");
		return ret;
	}

	struct vaccel_session sess;
	ret = create_session(&sess);
	if (ret)
		return ret;

	vaccel_info("Registering model %lld with session %u", model.id,
		    sess.session_id);

	ret = vaccel_resource_register(&sess, &model);
	if (ret) {
		vaccel_error("Could not register model to session");
		return ret;
	}

	vaccel_info("Unregistering model %lld from session %u", model.id,
		    sess.session_id);

	ret = vaccel_resource_unregister(&sess, &model);
	if (ret) {
		vaccel_error("Could not unregister model from session");
		return ret;
	}

	vaccel_info("Destroying model %lld", model.id);
	ret = vaccel_resource_destroy(&model);
	if (ret) {
		vaccel_error("Could not destroy model");
		return ret;
	}

	free(ptr);

	vaccel_info("Destroying session %u", sess.session_id);
	return destroy_session(&sess);
}

int main(int argc, char *argv[])
{
	if (argc != 2) {
		vaccel_info("usage: %s model_path", argv[0]);
		return 0;
	}

	vaccel_info("Testing model handling from path");
	int ret = create_from_path(argv[1]);
	if (ret)
		return ret;

	vaccel_info("Testing model handling from in memory data");
	return create_from_in_mem(argv[1]);
}
