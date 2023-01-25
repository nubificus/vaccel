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

static unsigned char *read_file(const char *path, size_t *len)
{
	struct stat buffer;
	int status, fd;

	fd = open(path, O_RDONLY);
	if (fd < 0)
	{
		perror("Could not open file");
		return NULL;
	}

	status = fstat(fd, &buffer);
	if (status < 0)
	{
		perror("Coult not stat file");
		return NULL;
	}

	unsigned char *buff = malloc(buffer.st_size);
	if (!buff)
	{
		close(fd);
		perror("malloc");
		return NULL;
	}

	size_t bytes = buffer.st_size;
	ssize_t ptr = 0;
	while (bytes)
	{
		ssize_t ret = read(fd, &buff[ptr], bytes);
		if (ret < 0)
		{
			perror("read");
			free(buff);
			close(fd);
			return NULL;
		}

		ptr += ret;
		bytes -= ret;
	}

	close(fd);

	*len = ptr;
	return buff;
}

int main(int argc, char *argv[])
{
	if (argc != 3)
	{
		fprintf(stderr, "usage: %s path_shared_object iterations\n", argv[0]);
		return 0;
	}

	int input;
	char out_text[512];
	char out_text2[512];

	struct vaccel_shared_object object;

	int ret = vaccel_shared_object_new(&object, argv[1]);
	if (ret)
	{
		fprintf(stderr, "Could not create shared object resource: %s",
				strerror(ret));
		exit(1);
	}

	struct vaccel_session sess;
	ret = vaccel_sess_init(&sess, 0);
	if (ret)
	{
		fprintf(stderr, "Could not create new shared object\n");
		exit(1);
	}
	printf("Initialized session with id: %u\n", sess.session_id);
	ret = vaccel_sess_register(&sess, object.resource);
	if (ret)
	{
		fprintf(stderr, "Could register shared object to session\n");
		exit(1);
	}

	struct vaccel_shared_object object2;
	size_t len;
	unsigned char *buff = read_file(argv[1], &len);
	if (!buff)
	{
		fprintf(stderr, "Could not read shared object file\n");
		exit(1);
	}

	ret = vaccel_shared_object_new_from_buffer(&object2, buff, len);
	if (ret)
	{
		fprintf(stderr, "Could not create shared object2 resource from buffer: %s\n",
				strerror(ret));
		exit(1);
	}

	ret = vaccel_sess_register(&sess, object2.resource);
	if (ret)
	{
		fprintf(stderr, "Could not register object 2 to session\n");
		exit(1);
	}

	input = 10; /* some random input value */
	struct vaccel_arg read[1] = {
		{.size = sizeof(input), .buf = &input}};
	struct vaccel_arg write[1] = {
		{.size = sizeof(out_text), .buf = out_text},
	};

	for (int i = 0; i < atoi(argv[2]); ++i)
	{
		ret = vaccel_exec_with_resource(&sess, &object, "mytestfunc", read, 1, write, 1);
		if (ret)
		{
			fprintf(stderr, "Could not run op: %d\n", ret);
			goto close_session;
		}
	}
	printf("output: %s\n", out_text);

	struct vaccel_arg read_2[1] = {
		{.size = sizeof(input), .buf = &input}};
	struct vaccel_arg write_2[1] = {
		{.size = sizeof(out_text2), .buf = out_text2},
	};

	for (int i = 0; i < atoi(argv[2]); ++i)
	{
		ret = vaccel_exec_with_resource(&sess, &object2, "mytestfunc", read_2, 1, write_2, 1);
		if (ret)
		{
			fprintf(stderr, "Could not run op: %d\n", ret);
			goto close_session;
		}
	}

	printf("output: %s\n", out_text2);
	ret = vaccel_sess_unregister(&sess, object.resource);
	if (ret) {
		fprintf(stderr, "Could not unregister object from session\n");
		exit(1);
	}

	ret = vaccel_sess_unregister(&sess, object2.resource);
	if (ret) {
		fprintf(stderr, "Could not unregister object 2 from session\n");
		exit(1);
	}
	

	ret = vaccel_shared_object_destroy(&object);
	if (ret) {
		fprintf(stderr, "Could not destroy object\n");
		exit(1);
	}

	ret = vaccel_shared_object_destroy(&object2);
	if (ret) {
		fprintf(stderr, "Could not destroy object2\n");
		exit(1);
	}

	ret = vaccel_sess_free(&sess);
	if (ret)
	{
		fprintf(stderr, "Could not close session\n");
		exit(1);
	}

	free(buff);

	return 0;

close_session:
	ret = vaccel_sess_unregister(&sess, object.resource);
	if (ret) {
		fprintf(stderr, "Could not unregister object from session\n");
		exit(1);
	}

	ret = vaccel_sess_unregister(&sess, object2.resource);
	if (ret) {
		fprintf(stderr, "Could not unregister object 2 from session\n");
		exit(1);
	}
	

	ret = vaccel_shared_object_destroy(&object);
	if (ret) {
		fprintf(stderr, "Could not destroy object\n");
		exit(1);
	}

	ret = vaccel_shared_object_destroy(&object2);
	if (ret) {
		fprintf(stderr, "Could not destroy object2\n");
		exit(1);
	}

	if (vaccel_sess_free(&sess) != VACCEL_OK)
	{
		fprintf(stderr, "Could not clear session\n");
		return 1;
	}

	free(buff);
	return ret;
}
