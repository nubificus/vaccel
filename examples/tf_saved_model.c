// SPDX-License-Identifier: Apache-2.0

#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../src/utils/fs.h"
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
	vaccel_info("Creating new SavedModel handle");
	struct vaccel_resource model;

	int ret = vaccel_resource_init(&model, path, VACCEL_RESOURCE_MODEL);
	if (ret) {
		vaccel_error("Could not create resource");
	}

	vaccel_id_t model_id = model.id;
	vaccel_info("Registered new resource: %lld", model_id);

	struct vaccel_session sess;
	ret = create_session(&sess);
	if (ret)
		return ret;

	vaccel_info("Registering model %lld with session %u", model_id,
		    sess.session_id);

	ret = vaccel_resource_register(&model, &sess);
	if (ret) {
		vaccel_error("Could not register model to session");
		return ret;
	}

	vaccel_info("Unregistering model %lld from session %u", model_id,
		    sess.session_id);

	ret = vaccel_resource_unregister(&model, &sess);
	if (ret) {
		vaccel_error("Could not unregister model from session");
		return ret;
	}

	vaccel_info("Destroying model %lld", model_id);
	ret = vaccel_resource_release(&model);
	if (ret) {
		vaccel_error("Could not destroy model");
		return ret;
	}

	vaccel_info("Destroying session %u", sess.session_id);
	return destroy_session(&sess);
}

int create_from_in_mem(const char *path)
{
	vaccel_info("Creating new SavedModel handle");
	struct vaccel_resource model;

	char path1[200] = { '\0' };
	char path2[200] = { '\0' };

	sprintf(path1, "%s/%s", path, "saved_model.pb");
	sprintf(path2, "%s/%s", path, "variables/variables.index");

	const char *paths[] = { path1, path2, path2 };

	int ret = vaccel_resource_init_multi(&model, paths, 3,
					     VACCEL_RESOURCE_MODEL);
	if (ret) {
		vaccel_error("Could not create model resource");
		return ret;
	}

	vaccel_id_t model_id = model.id;
	vaccel_info("Registered new resource: %lld", model_id);

	struct vaccel_session sess;
	ret = create_session(&sess);
	if (ret)
		return ret;

	vaccel_info("Registering model %lld with session %u", model_id,
		    sess.session_id);

	ret = vaccel_resource_register(&model, &sess);
	if (ret) {
		vaccel_error("Could not register model to session");
		return ret;
	}

	vaccel_info("Unregistering model %lld from session %u", model_id,
		    sess.session_id);

	ret = vaccel_resource_unregister(&model, &sess);
	if (ret) {
		vaccel_error("Could not unregister model from session");
		return ret;
	}

	vaccel_info("Destroying model %lld", model_id);
	ret = vaccel_resource_release(&model);
	if (ret) {
		vaccel_error("Could not destroy model");
		return ret;
	}

	vaccel_info("Destroying session %u", sess.session_id);
	return destroy_session(&sess);
}

int main(int argc, char *argv[])
{
	if (argc != 2) {
		vaccel_info("usage: %s saved_model_path", argv[0]);
		return 0;
	}

	vaccel_info("Testing SavedModel handling from path");
	int ret = create_from_path(argv[1]);
	if (ret)
		return ret;

	vaccel_info("Testing SavedModel handling from in memory data");
	return create_from_in_mem(argv[1]);
}
