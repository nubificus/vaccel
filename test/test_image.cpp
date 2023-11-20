#include <catch2/catch_test_macros.hpp>

#include <atomic>

using atomic_int = std::atomic<int>;
using atomic_uint = std::atomic<unsigned int>;

extern "C"{
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <vaccel.h>
#include "session.h"
}


extern "C"{

int read_file(const char *filename, char **img, size_t *img_size) {
    int fd;
    long bytes = 0;

    fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("open: ");
        return 1;
    }

    struct stat info;
    fstat(fd, &info);
    fprintf(stdout, "Image size: %luB\n", info.st_size);

    char *buf = (char *)malloc(info.st_size);
    if (!buf) {
        fprintf(stderr, "Could not allocate memory for image\n");
        goto free_buff;
    }

    do {
        int ret = read(fd, buf, info.st_size);
        if (ret < 0) {
            perror("Error while reading image: ");
            goto close_file;
        }
        bytes += ret;
    } while (bytes < info.st_size);

    if (bytes < info.st_size) {
        fprintf(stderr, "Could not read image\n");
        goto close_file;
    }

    *img = buf;
    *img_size = info.st_size;
    close(fd);

    return 0;

close_file:
    close(fd);
free_buff:
    free(buf);
    return 1;
}

}
TEST_CASE("classify")
{   
    char program_name[] = "program_name";
    char file_path[] = "../../test/images/example.jpg";
    char iterations[] = "2";
    char *argv[] = {program_name, file_path, iterations};

    int ret;
	char *image;
    size_t image_size;
	char out_text[512], out_imagename[512];
	struct vaccel_session sess;


    ret = vaccel_sess_init(&sess, 0);

    REQUIRE(ret == VACCEL_OK);

    ret = read_file(file_path, &image, &image_size);

    REQUIRE(ret == 0);

	for (int i = 0; i < atoi(argv[2]); ++i) {
		ret = vaccel_image_classification(&sess, image, (unsigned char*)out_text, (unsigned char*)out_imagename,
				image_size, sizeof(out_text), sizeof(out_imagename));

        if (ret){
            fprintf(stderr, "Could not run op: %d\n", ret);
            goto close_session;
        }

        if (i == 0)
                printf("classification tags: %s\n", out_text);

    }


close_session:
        free(image);
        if (vaccel_sess_free(&sess) != VACCEL_OK) {
            fprintf(stderr, "Could not clear session\n");
            printf("%d\n", 1);
	    }

	    printf("%d\n", ret);
}



TEST_CASE("classify_generic")
{   
    char program_name[] = "program_name";
    char file_path[] = "../../test/images/example.jpg";
    char iterations[] = "2";
    char *argv[] = {program_name, file_path, iterations};

    int ret;
	char *image;
    size_t image_size;
	char out_text[512], out_imagename[512];
	struct vaccel_session sess;


    ret = vaccel_sess_init(&sess, 0);

    REQUIRE(ret == VACCEL_OK);

    ret = read_file(file_path, &image, &image_size);

    REQUIRE(ret == 0);

    uint32_t image_size_uint32 = 0;
    if (image_size <= UINT32_MAX) {
        image_size_uint32 = static_cast<uint32_t>(image_size);
    } else {
        REQUIRE(1==2); // lets fail the test here
    }

    enum vaccel_op_type op_type = VACCEL_IMG_CLASS;
    struct vaccel_arg read[2] = {
		{ .size = sizeof(enum vaccel_op_type), .buf = &op_type},
		{ .size = image_size_uint32, .buf = image }};

    struct vaccel_arg write[2] = {
		{ .size = sizeof(out_text), .buf = out_text },
		{ .size = sizeof(out_imagename), .buf = out_imagename }};

	for (int i = 0; i < atoi(argv[2]); ++i) {
		ret = vaccel_genop(&sess, read, 2, write, 2);
		if (ret) {
			fprintf(stderr, "Could not run op: %d\n", ret);
			goto close_session;
		}

		if (i == 0)
			printf("classification tags: %s\n", out_text);
	}


close_session:
        free(image);
        if (vaccel_sess_free(&sess) != VACCEL_OK) {
            fprintf(stderr, "Could not clear session\n");
            printf("%d\n", 1);
	    }

	    printf("%d\n", ret);
}

TEST_CASE("depth")
{

    char program_name[] = "program_name";
    char file_path[] = "../../test/images/example.jpg";
    char iterations[] = "2";
    char *argv[] = {program_name, file_path, iterations};
    int ret;
	char *image;
    size_t image_size;
	char out_imagename[512];
	struct vaccel_session sess;

    ret = vaccel_sess_init(&sess, 0);
    REQUIRE(ret == VACCEL_OK);

    ret = read_file(argv[1], &image, &image_size);
    REQUIRE(ret == VACCEL_OK);

    for (int i = 0; i < atoi(argv[2]); ++i) {
		ret = vaccel_image_depth(&sess, image, (unsigned char*)out_imagename,
				image_size, sizeof(out_imagename));

		if (ret) {
			fprintf(stderr, "Could not run op: %d\n", ret);
			goto close_session;
		}

		if (i == 0)
			printf("depth estimation imagename: %s\n", out_imagename);
	}


close_session:
	free(image);
	if (vaccel_sess_free(&sess) != VACCEL_OK) {
		fprintf(stderr, "Could not clear session\n");
		printf("%d\n", 1);
	}

	printf("%d\n", ret);

}

TEST_CASE("depth_generic")
{   
    char program_name[] = "program_name";
    char file_path[] = "../../test/images/example.jpg";
    char iterations[] = "2";
    char *argv[] = {program_name, file_path, iterations};

	int ret;
	char *image;
    size_t image_size;
	char out_imagename[512];
	struct vaccel_session sess;

    ret = vaccel_sess_init(&sess, 0);
    REQUIRE(ret == VACCEL_OK);

    ret = read_file(file_path, &image, &image_size);
    REQUIRE(ret == 0);

    uint32_t image_size_uint32 = 0;
    if (image_size <= UINT32_MAX) {
        image_size_uint32 = static_cast<uint32_t>(image_size);
    } else {
        REQUIRE(1==2); // lets fail the test here
    }

    enum vaccel_op_type op_type = VACCEL_IMG_DEPTH;
    struct vaccel_arg read[2] = {
		{ .size = sizeof(enum vaccel_op_type), .buf = &op_type},
		{ .size = image_size_uint32, .buf = image }};

	struct vaccel_arg write[1] = {
		{ .size = sizeof(out_imagename), .buf = out_imagename }};

	for (int i = 0; i < atoi(argv[2]); ++i) {
		ret = vaccel_genop(&sess, read, 2, write, 1);
		if (ret) {
			fprintf(stderr, "Could not run op: %d\n", ret);
			goto close_session;
        }
	}


close_session:
        free(image);
        if (vaccel_sess_free(&sess) != VACCEL_OK) {
            fprintf(stderr, "Could not clear session\n");
            printf("%d\n", 1);
	    }

	    printf("%d\n", ret);
}


TEST_CASE("detect")
{

    char program_name[] = "program_name";
    char file_path[] = "../../test/images/example.jpg";
    char iterations[] = "2";
    char *argv[] = {program_name, file_path, iterations};

    int ret;
	char *image;
    size_t image_size;
	char out_imagename[512];
	struct vaccel_session sess;

    ret = vaccel_sess_init(&sess, 0);
    REQUIRE(ret == VACCEL_OK);

    ret = read_file(argv[1], &image, &image_size);
    REQUIRE(ret == VACCEL_OK);

	for (int i = 0; i < atoi(argv[2]); ++i) {
		ret = vaccel_image_detection(&sess, image, (unsigned char*)out_imagename,
				image_size, sizeof(out_imagename));

		if (ret) {
			fprintf(stderr, "Could not run op: %d\n", ret);
			goto close_session;
		}

		if (i == 0)
			printf("detection imagename: %s\n", out_imagename);
	}

close_session:
	free(image);
	if (vaccel_sess_free(&sess) != VACCEL_OK) {
		fprintf(stderr, "Could not clear session\n");
		printf("%d\n", 1);
	}

	printf("%d\n", ret);

}


TEST_CASE("detect_generic")
{

    char program_name[] = "program_name";
    char file_path[] = "../../test/images/example.jpg";
    char iterations[] = "2";
    char *argv[] = {program_name, file_path, iterations};

    int ret;
	char *image;
    size_t image_size;
	char out_imagename[512];
	struct vaccel_session sess;

    ret = vaccel_sess_init(&sess, 0);
    REQUIRE(ret == VACCEL_OK);

    ret = read_file(argv[1], &image, &image_size);
    REQUIRE(ret == VACCEL_OK);

    uint32_t image_size_uint32 = 0;
    if (image_size <= UINT32_MAX) {
        image_size_uint32 = static_cast<uint32_t>(image_size);
    } else {
        REQUIRE(1==2); // lets fail the test here
    }

	enum vaccel_op_type op_type = VACCEL_IMG_DETEC;
	struct vaccel_arg read[2] = {
		{ .size = sizeof(enum vaccel_op_type), .buf = &op_type},
		{ .size = image_size_uint32, .buf = image }
	};
	struct vaccel_arg write[1] = {
		{ .size = sizeof(out_imagename), .buf = out_imagename },
	};

	for (int i = 0; i < atoi(argv[2]); ++i) {
		ret = vaccel_genop(&sess, read, 2, write, 1);
		if (ret) {
			fprintf(stderr, "Could not run op: %d\n", ret);
			goto close_session;
		}
	}

close_session:
	free(image);
	if (vaccel_sess_free(&sess) != VACCEL_OK) {
		fprintf(stderr, "Could not clear session\n");
		printf("%d\n", 1);
	}

	printf("%d\n", ret);

}

TEST_CASE("pose")
{

    char program_name[] = "program_name";
    char file_path[] = "../../test/images/example.jpg";
    char iterations[] = "2";
    char *argv[] = {program_name, file_path, iterations};

    int ret;
	char *image;
    size_t image_size;
	char out_imagename[512];
	struct vaccel_session sess;

    ret = vaccel_sess_init(&sess, 0);
    REQUIRE(ret == VACCEL_OK);

    ret = read_file(argv[1], &image, &image_size);
    REQUIRE(ret == VACCEL_OK);

	for (int i = 0; i < atoi(argv[2]); ++i) {
		ret = vaccel_image_pose(&sess, image, (unsigned char*)out_imagename,
				image_size, sizeof(out_imagename));

		if (ret) {
			fprintf(stderr, "Could not run op: %d\n", ret);
			goto close_session;
		}

		if (i == 0)
			printf("pose estimation imagename: %s\n", out_imagename);
    }

close_session:
	free(image);
	if (vaccel_sess_free(&sess) != VACCEL_OK) {
		fprintf(stderr, "Could not clear session\n");
		printf("%d\n", 1);
	}

	printf("%d\n", ret);

}


TEST_CASE("pose_generic")
{

    char program_name[] = "program_name";
    char file_path[] = "../../test/images/example.jpg";
    char iterations[] = "2";
    char *argv[] = {program_name, file_path, iterations};

    int ret;
	char *image;
    size_t image_size;
	char out_imagename[512];
	struct vaccel_session sess;

    ret = vaccel_sess_init(&sess, 0);
    REQUIRE(ret == VACCEL_OK);

    ret = read_file(argv[1], &image, &image_size);
    REQUIRE(ret == VACCEL_OK);

    uint32_t image_size_uint32 = 0;
    if (image_size <= UINT32_MAX) {
        image_size_uint32 = static_cast<uint32_t>(image_size);
    } else {
        REQUIRE(1==2); // lets fail the test here
    }

	enum vaccel_op_type op_type = VACCEL_IMG_POSE;
	struct vaccel_arg read[2] = {
		{ .size = sizeof(enum vaccel_op_type), .buf = &op_type},
		{ .size = image_size_uint32, .buf = image }
	};
	struct vaccel_arg write[1] = {
		{ .size = sizeof(out_imagename), .buf = out_imagename },
	};

	for (int i = 0; i < atoi(argv[2]); ++i) {
		ret = vaccel_genop(&sess, read, 2, write, 1);
		if (ret) {
			fprintf(stderr, "Could not run op: %d\n", ret);
			goto close_session;
		}
	}

close_session:
	free(image);
	if (vaccel_sess_free(&sess) != VACCEL_OK) {
		fprintf(stderr, "Could not clear session\n");
		printf("%d\n", 1);
	}

	printf("%d\n", ret);

}


TEST_CASE("segmentation")
{

    char program_name[] = "program_name";
    char file_path[] = "../../test/images/example.jpg";
    char iterations[] = "2";
    char *argv[] = {program_name, file_path, iterations};

    int ret;
	char *image;
    size_t image_size;
	char out_imagename[512];
	struct vaccel_session sess;

    ret = vaccel_sess_init(&sess, 0);
    REQUIRE(ret == VACCEL_OK);

    ret = read_file(argv[1], &image, &image_size);
    REQUIRE(ret == VACCEL_OK);

	for (int i = 0; i < atoi(argv[2]); ++i) {
		ret = vaccel_image_segmentation(&sess, image, (unsigned char*)out_imagename,
				image_size, sizeof(out_imagename));

		if (ret) {
			fprintf(stderr, "Could not run op: %d\n", ret);
			goto close_session;
		}

		if (i == 0)
			printf("pose estimation imagename: %s\n", out_imagename);
    }

close_session:
	free(image);
	if (vaccel_sess_free(&sess) != VACCEL_OK) {
		fprintf(stderr, "Could not clear session\n");
		printf("%d\n", 1);
	}

	printf("%d\n", ret);

}


TEST_CASE("segmentation_generic")
{

    char program_name[] = "program_name";
    char file_path[] = "../../test/images/example.jpg";
    char iterations[] = "2";
    char *argv[] = {program_name, file_path, iterations};

    int ret;
	char *image;
    size_t image_size;
	char out_imagename[512];
	struct vaccel_session sess;

    ret = vaccel_sess_init(&sess, 0);
    REQUIRE(ret == VACCEL_OK);

    ret = read_file(argv[1], &image, &image_size);
    REQUIRE(ret == VACCEL_OK);

    uint32_t image_size_uint32 = 0;
    if (image_size <= UINT32_MAX) {
        image_size_uint32 = static_cast<uint32_t>(image_size);
    } else {
        REQUIRE(1==2); // lets fail the test here
    }

	enum vaccel_op_type op_type = VACCEL_IMG_SEGME;
	struct vaccel_arg read[2] = {
		{ .size = sizeof(enum vaccel_op_type), .buf = &op_type},
		{ .size = image_size_uint32, .buf = image }
	};
	struct vaccel_arg write[1] = {
		{ .size = sizeof(out_imagename), .buf = out_imagename },
	};

	for (int i = 0; i < atoi(argv[2]); ++i) {
		ret = vaccel_genop(&sess, read, 2, write, 1);
		if (ret) {
			fprintf(stderr, "Could not run op: %d\n", ret);
			goto close_session;
		}
	}

close_session:
	free(image);
	if (vaccel_sess_free(&sess) != VACCEL_OK) {
		fprintf(stderr, "Could not clear session\n");
		printf("%d\n", 1);
	}

	printf("%d\n", ret);

}