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
#include <string.h>
#include <unistd.h>

#include <vaccel.h>
#include "../src/utils.h"

int main(int argc, char *argv[])
{
	int ret, iter = 1;
	char *file;
	size_t file_size;
	struct vaccel_session sess;
	struct vaccel_arg args[2];

	if (argc < 3) {
		fprintf(stderr, "Usage: %s <time (ms)> <file> [<iterations>]\n", argv[0]);
		return 0;
	}
	if (argc == 4)
		iter = atoi(argv[3]);

	ret = vaccel_sess_init(&sess, 0);
	if (ret != VACCEL_OK) {
		fprintf(stderr, "Could not initialize session\n");
		return 1;
	}

	printf("Initialized session with id: %u\n", sess.session_id);

	ret = read_file(argv[2], (void **)&file, &file_size);
	if (ret)
		goto close_session;

	char lib[8] = "mbench";
	char func[8] = "mbench";
	memset(args, 0, sizeof(args));
	args[0].size = sizeof(int);
	args[0].buf = argv[1];
	args[1].size = file_size;
	args[1].buf = file;

	for (int i = 0; i < iter; ++i) {
		ret = vaccel_exec(&sess, lib, func, &args[0], 2, NULL, 0);
		if (ret) {
			fprintf(stderr, "Could not run op: %d\n", ret);
			goto close_session;
		}
	}


close_session:
	free(file);
	if (vaccel_sess_free(&sess) != VACCEL_OK) {
		fprintf(stderr, "Could not clear session\n");
		return 1;
	}

	return ret;
}
