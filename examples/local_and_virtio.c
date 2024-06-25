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
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <vaccel.h>
#include "../src/utils.h"

int main(int argc, char *argv[])
{
	if (argc != 3) {
		fprintf(stderr, "usage: %s path_shared_object iterations\n",
			argv[0]);
		return 0;
	}

	int input;
	int output1;
	struct vaccel_session virtio_sess, local_sess;
	struct vaccel_session *sess;
	struct vaccel_shared_object object;
	int ret;

	ret = vaccel_shared_object_new(&object, argv[1]);
	if (ret) {
		vaccel_error("Could not create shared object resource: %s",
			     strerror(ret));
		exit(1);
	}

	if (vaccel_sess_init(&virtio_sess, VACCEL_REMOTE)) {
		vaccel_error("Could not create virtio session");
		exit(1);
	}

	printf("Initialized virtio session with id: %u\n",
	       virtio_sess.session_id);

	if (vaccel_sess_init(&local_sess, 0)) {
		vaccel_error("Could not create local session");
		exit(1);
	}

	printf("Initialized local session with id: %u\n",
	       local_sess.session_id);

	if (vaccel_sess_register(&local_sess, object.resource)) {
		vaccel_error("Could register shared object to local session");
		exit(1);
	}

	if (vaccel_sess_register(&virtio_sess, object.resource)) {
		vaccel_error("Could register shared object to virtio session");
		exit(1);
	}

	input = 10; /* some random input value */
	struct vaccel_arg read[1] = {
		{ .size = sizeof(input), .buf = &input, .argtype = 42 }
	};
	struct vaccel_arg write[1] = {
		{ .size = sizeof(output1), .buf = &output1, .argtype = 42 },
	};

	for (int i = 0; i < atoi(argv[2]); ++i) {
		sess = i % 2 ? &local_sess : &virtio_sess;

		ret = vaccel_exec_with_resource(sess, &object, "mytestfunc",
						read, 1, write, 1);
		if (ret) {
			vaccel_error("Could not run op: %d", ret);
			break;
		}
		printf("output1(2x%d): %d\n", input, output1);
		output1 = -1;
	}

	if (vaccel_sess_unregister(&local_sess, object.resource)) {
		vaccel_error("Could not unregister object from local session");
		exit(1);
	}

	if (vaccel_sess_unregister(&virtio_sess, object.resource)) {
		vaccel_error("Could not unregister object from virtio session");
		exit(1);
	}

	if (vaccel_shared_object_destroy(&object)) {
		vaccel_error("Could not destroy object");
		exit(1);
	}

	if (vaccel_sess_free(&local_sess)) {
		vaccel_error("Could not close session");
		exit(1);
	}

	if (vaccel_sess_free(&virtio_sess)) {
		vaccel_error("Could not close session");
		exit(1);
	}

	return 0;
}
