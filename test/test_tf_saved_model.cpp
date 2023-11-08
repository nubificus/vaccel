#include <catch2/catch_test_macros.hpp>

#include <atomic>

using atomic_int = std::atomic<int>;
using atomic_uint = std::atomic<unsigned int>;

extern "C"{
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include <vaccel.h>
#include "session.h"
#include "tf_model.h"
#include "tf_saved_model.h"
#include "log.h"

}

extern "C"
{

static unsigned char *read_file(const char *path, size_t *len)
{
	struct stat stat;
	int status, fd;

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		perror("Could not open file");
		return NULL;
	}

	status = fstat(fd, &stat);
	if (status < 0) {
		perror("Could not stat file");
		return NULL;
	}

    unsigned char *ptr = static_cast<unsigned char*>(mmap(NULL, stat.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0));
	close(fd);

	if(!ptr)
		return NULL;

	*len = stat.st_size;

	return ptr;
}

static unsigned char *read_file_from_dir(
	const char *dir,
	const char *path,
	size_t *len
) {
	char fpath[1024];

	snprintf(fpath, 1024, "%s/%s", dir, path);
	unsigned char *ptr = read_file(fpath, len);
	if (!ptr)
		vaccel_error("Could not mmap %s", fpath);

	return ptr;
}

}

TEST_CASE("saved_tf_model_from_memory")
{   
    int ret;
    const char *path = "../../test/models/tf/lstm2/";

    struct vaccel_tf_saved_model *model = vaccel_tf_saved_model_new();
    REQUIRE(model);

    size_t len;
    unsigned char *ptr = read_file_from_dir(path, "saved_model.pb", &len);
    REQUIRE(ptr);

    ret = vaccel_tf_saved_model_set_model(model, ptr, len);
    REQUIRE(ret == VACCEL_OK);

    ptr = read_file_from_dir(path, "variables/variables.index", &len);
    REQUIRE(ptr);

    ret = vaccel_tf_saved_model_set_checkpoint(model, ptr, len);
    REQUIRE(ptr);

    ptr = read_file_from_dir(path, "variables/variables.index", &len);
    REQUIRE(ptr);
    ret = vaccel_tf_saved_model_set_var_index(model, ptr, len);
    REQUIRE(ret == VACCEL_OK);


    ret = vaccel_tf_saved_model_register(model);
    REQUIRE(ret == VACCEL_OK);

    vaccel_id_t model_id = vaccel_tf_saved_model_id(model);
    vaccel_info("Registered new resource: %ld", model_id);

    struct vaccel_session sess;
    ret = vaccel_sess_init(&sess, 0);
    REQUIRE(ret == VACCEL_OK);

    vaccel_info("Registering model %ld with session %u", model_id,
			sess.session_id);

    ret = vaccel_sess_register(&sess, model->resource);
    REQUIRE(ret == VACCEL_OK);

    ret = vaccel_sess_unregister(&sess, model->resource);
    REQUIRE(ret == VACCEL_OK);

    ret = vaccel_tf_saved_model_destroy(model);
    REQUIRE(ret == VACCEL_OK);

    ret = vaccel_sess_free(&sess);
    REQUIRE(ret == VACCEL_OK);
}

TEST_CASE("saved_model_from_file")
{

    int ret;
    const char *path = "../../test/models/tf/lstm2/";

    struct vaccel_tf_saved_model *model = vaccel_tf_saved_model_new();
    REQUIRE(model);

    ret = vaccel_tf_saved_model_set_path(model, path);
    REQUIRE(ret == VACCEL_OK);

    ret = vaccel_tf_saved_model_register(model);
    REQUIRE(ret == VACCEL_OK);

    vaccel_id_t model_id = vaccel_tf_saved_model_id(model);
    vaccel_info("Registered new resource: %ld", model_id);

    struct vaccel_session sess;
    ret = vaccel_sess_init(&sess, 0);
    REQUIRE(ret == VACCEL_OK);

    ret = vaccel_sess_register(&sess, model->resource);
    REQUIRE(ret == VACCEL_OK);

    ret = vaccel_sess_unregister(&sess, model->resource);
    REQUIRE(ret == VACCEL_OK);

    ret = vaccel_tf_saved_model_destroy(model);
    REQUIRE(ret == VACCEL_OK);

    ret = vaccel_sess_free(&sess);
    REQUIRE(ret == VACCEL_OK);
}
