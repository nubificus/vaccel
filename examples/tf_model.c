/*
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include <vaccel.h>
#include "../src/utils.h"

int main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "usage: %s path_to_tf_model\n", argv[0]);
		return 0;
	}

	struct vaccel_tf_model model;

	int ret = vaccel_tf_model_new(&model, argv[1]);
	if (ret) {
		fprintf(stderr, "Could not create TF model resource: %s",
				strerror(ret));
		exit(1);
	}

	struct vaccel_session sess;
	ret = vaccel_sess_init(&sess, 0);
	if (ret) {
		fprintf(stderr, "Could not create new session\n");
		exit(1);
	}

	ret = vaccel_sess_register(&sess, model.resource);
	if (ret) {
		fprintf(stderr, "Could register model to session\n");
		exit(1);
	}

	struct vaccel_tf_model model2;
	size_t len;
	unsigned char *buff;
	ret = read_file(argv[1], (void **)&buff, &len);
	if (ret) {
		fprintf(stderr, "Could not read model file\n");
		exit(1);
	}
		

	ret = vaccel_tf_model_new_from_buffer(&model2, buff, len);
	if (ret) {
		fprintf(stderr, "Could not create TF model resource from buffer: %s\n",
				strerror(ret));
		exit(1);
	}

	ret = vaccel_sess_register(&sess, model2.resource);
	if (ret) {
		fprintf(stderr, "Could not register model 2 to session\n");
		exit(1);
	}

	ret = vaccel_sess_unregister(&sess, model.resource);
	if (ret) {
		fprintf(stderr, "Could not unregister model from session\n");
		exit(1);
	}

	ret = vaccel_sess_unregister(&sess, model2.resource);
	if (ret) {
		fprintf(stderr, "Could not unregister model 2 from session\n");
		exit(1);
	}
	

	ret = vaccel_tf_model_destroy(&model);
	if (ret) {
		fprintf(stderr, "Could not destroy model\n");
		exit(1);
	}

	ret = vaccel_tf_model_destroy(&model2);
	if (ret) {
		fprintf(stderr, "Could not destroy model2\n");
		exit(1);
	}

	ret = vaccel_sess_free(&sess);
	if (ret) {
		fprintf(stderr, "Could not close session\n");
		exit(1);
	}

	free(buff);

	return 0;
}
