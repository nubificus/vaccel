#include <catch2/catch_test_macros.hpp>

#include <atomic>

using atomic_int = std::atomic<int>;
using atomic_uint = std::atomic<unsigned int>;


extern "C"
{
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <vaccel.h>
#include "session.h"
#include "plugin.h"
#include "shared_object.h"
}

TEST_CASE("exec")
{
    int ret;
	struct vaccel_session sess;
	int input;
	char out_text[512];
    sess.hint = VACCEL_PLUGIN_DEBUG;
    char iterations[] = "2";
    
    ret = vaccel_sess_init(&sess, sess.hint);
    REQUIRE(ret == VACCEL_OK);

    input = 10;	

	struct vaccel_arg read[1] = {
		{.size = sizeof(input),.buf = &input}};
	struct vaccel_arg write[1] = {
		{.size = sizeof(out_text),.buf = out_text}};

	for (int i = 0; i < atoi(iterations); ++i) {
		ret = vaccel_exec(&sess, "../plugins/noop/libvaccel-noop.so",
				"mytestfunc", read, 1, write, 1);
		if (ret) {
			fprintf(stderr, "Could not run op: %d\n", ret);
			goto close_session;
		}
	}


close_session:
	if (vaccel_sess_free(&sess) != VACCEL_OK) {
		fprintf(stderr, "Could not clear session\n");
		printf("%d\n", 1);
	}

	printf("%d\n", ret);
}

TEST_CASE("exec_generic")
{
	int ret;
	struct vaccel_session sess;
	int input;
	char out_text[512];
    char iterations[] = "2";

    ret = vaccel_sess_init(&sess, 0);
    REQUIRE(ret == VACCEL_OK);

    input = 10; 
    enum vaccel_op_type op_type = VACCEL_EXEC;

    const char plugin_path[] = "../plugins/noop/libvaccel-noop.so";
    const char function_name[] = "mytestfunc";


	struct vaccel_arg read[4] = {
		{.size = sizeof(uint8_t),.buf = &op_type},
		{.size = sizeof(plugin_path) ,.buf = (void*)plugin_path},
		{.size = sizeof(function_name), .buf = (void*)function_name },
		{.size = sizeof(input),.buf = &input}
	};
	
    struct vaccel_arg write[1] = {
		{.size = sizeof(out_text),.buf = out_text},
	};

	for (int i = 0; i < atoi(iterations); ++i) {
		ret = vaccel_genop(&sess, read, 4, write, 1);
		if (ret) {
			fprintf(stderr, "Could not run op: %d\n", ret);
			goto close_session;
		}
	}
	printf("output: %s\n", out_text);

close_session:
	if (vaccel_sess_free(&sess) != VACCEL_OK) {
		fprintf(stderr, "Could not clear session\n");
		printf("%d\n", 1);
	}

	printf("%d\n", ret);
}


extern "C"
{
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

	unsigned char *buff = static_cast<unsigned char*>(malloc(buffer.st_size));
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
}


TEST_CASE("exec_with_resources")
{

	int input;
	char out_text[512];
	char out_text2[512];
    char iterations[] = "1";
    const char plugin_path[] = "../plugins/noop/libvaccel-noop.so";

    struct vaccel_shared_object object;

    int ret = vaccel_shared_object_new(&object, plugin_path);
    REQUIRE(ret == VACCEL_OK);

    struct vaccel_session sess;
    ret = vaccel_sess_init(&sess, 0);
    REQUIRE(ret == VACCEL_OK);

    ret = vaccel_sess_register(&sess, object.resource);
    REQUIRE(ret == VACCEL_OK);

    struct vaccel_shared_object object2;
    size_t len;
    unsigned char *buff = read_file(plugin_path, &len);
    REQUIRE(buff);

    ret = vaccel_shared_object_new_from_buffer(&object2, buff, len);
    REQUIRE(ret == VACCEL_OK);

    ret = vaccel_sess_register(&sess, object2.resource);
    REQUIRE(ret == VACCEL_OK);
    
    input = 10;
	struct vaccel_arg read[1] = {
		{.size = sizeof(input), .buf = &input}};
	struct vaccel_arg write[1] = {
		{.size = sizeof(out_text), .buf = out_text},
	};


	for (int i = 0; i < atoi(iterations); ++i)
	{
		ret = vaccel_exec_with_resource(&sess, &object, "mytestfunc", read, 1, write, 1);
		if (ret)
		{
			fprintf(stderr, "Could not run op: %d\n", ret);
			break;
		}
	}
    REQUIRE(ret == VACCEL_OK);

	struct vaccel_arg read_2[1] = {
		{.size = sizeof(input), .buf = &input}};
	struct vaccel_arg write_2[1] = {
		{.size = sizeof(out_text2), .buf = out_text2},
	};

	for (int i = 0; i < atoi(iterations); ++i)
	{
		ret = vaccel_exec_with_resource(&sess, &object2, "mytestfunc", read_2, 1, write_2, 1);
		if (ret)
		{
			fprintf(stderr, "Could not run op: %d\n", ret);
			break;
		}
	}
    REQUIRE(ret == VACCEL_OK);
    
    ret = vaccel_sess_unregister(&sess, object.resource);
    REQUIRE(ret == VACCEL_OK);

    ret = vaccel_sess_unregister(&sess, object2.resource);
    REQUIRE(ret == VACCEL_OK);

    ret = vaccel_shared_object_destroy(&object);
    REQUIRE(ret == VACCEL_OK);

    ret = vaccel_shared_object_destroy(&object2);
    REQUIRE(ret == VACCEL_OK);

    ret = vaccel_sess_free(&sess);
    REQUIRE(ret == VACCEL_OK);

    free(buff);
}
