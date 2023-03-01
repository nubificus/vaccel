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

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <vaccel.h>

int main(int argc, char *argv[])
{
	int ret;
	struct vaccel_session sess;
	int input;
	char out_text[512];

	if (argc < 2) {
		fprintf(stderr, "You must specify the number of iterations\n");
		return 1;
	}

	sess.hint = VACCEL_PLUGIN_DEBUG;
	ret = vaccel_sess_init(&sess, sess.hint);
	if (ret != VACCEL_OK) {
		fprintf(stderr, "Could not initialize session\n");
		return 1;
	}

	printf("Initialized session with id: %u\n", sess.session_id);

	input = 10;		/* some random input value */
	struct vaccel_arg read[1] = {
		{.size = sizeof(input),.buf = &input}
	};
	struct vaccel_arg write[1] = {
		{.size = sizeof(out_text),.buf = out_text},
	};

	//vaccel_get_plugins(&sess, 7);

	for (int i = 0; i < atoi(argv[1]); ++i) {
		ret =
		    vaccel_exec(&sess, "/usr/local/lib/libmytestlib.so",
				"mytestfunc", read, 1, write, 1);
		if (ret) {
			fprintf(stderr, "Could not run op: %d\n", ret);
			goto close_session;
		}
	}
	printf("output: %s\n", out_text);

 close_session:
	if (vaccel_sess_free(&sess) != VACCEL_OK) {
		fprintf(stderr, "Could not clear session\n");
		return 1;
	}

	return ret;
}
