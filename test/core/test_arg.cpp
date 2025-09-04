// SPDX-License-Identifier: Apache-2.0

/*
 * The code below performs unit testing to `arg` functions.
 */

#include "common/mydata.h"
#include "vaccel.h"
#include <catch.hpp>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

TEST_CASE("vaccel_arg_init", "[core][arg]")
{
	int ret;
	int32_t buf = 1;
	struct vaccel_arg arg;

	SECTION("empty buffer")
	{
		ret = vaccel_arg_init(&arg, nullptr, 0, VACCEL_ARG_RAW, 0);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(arg.buf == NULL);
		REQUIRE(arg.size == 0);
		REQUIRE(arg.type == VACCEL_ARG_RAW);
		REQUIRE(arg.custom_type_id == 0);
		REQUIRE(arg.owned == true);

		REQUIRE(vaccel_arg_release(&arg) == VACCEL_OK);
	}

	SECTION("zeroed buffer")
	{
		ret = vaccel_arg_init(&arg, nullptr, sizeof(buf),
				      VACCEL_ARG_RAW, 0);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(arg.buf);
		REQUIRE(arg.size == sizeof(buf));
		REQUIRE(arg.type == VACCEL_ARG_RAW);
		REQUIRE(arg.custom_type_id == 0);
		REQUIRE(arg.owned == true);

		REQUIRE(vaccel_arg_release(&arg) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_arg_init(nullptr, &buf, sizeof(buf),
				      VACCEL_ARG_RAW, 0);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_arg_init(&arg, &buf, 0, VACCEL_ARG_RAW, 0);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	ret = vaccel_arg_init(&arg, &buf, sizeof(buf), VACCEL_ARG_RAW, 0);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(*(int32_t *)arg.buf == 1);
	REQUIRE(arg.size == sizeof(buf));
	REQUIRE(arg.type == VACCEL_ARG_RAW);
	REQUIRE(arg.custom_type_id == 0);
	REQUIRE(arg.owned == true);

	REQUIRE(vaccel_arg_release(&arg) == VACCEL_OK);
}

TEST_CASE("vaccel_arg_init_from_buf", "[core][arg]")
{
	int ret;
	int32_t buf = 1;
	struct vaccel_arg arg;

	SECTION("empty buffer")
	{
		ret = vaccel_arg_init_from_buf(&arg, nullptr, 0, VACCEL_ARG_RAW,
					       0);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(arg.buf == NULL);
		REQUIRE(arg.size == 0);
		REQUIRE(arg.type == VACCEL_ARG_RAW);
		REQUIRE(arg.custom_type_id == 0);
		REQUIRE(arg.owned == false);

		REQUIRE(vaccel_arg_release(&arg) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_arg_init_from_buf(nullptr, &buf, sizeof(buf),
					       VACCEL_ARG_RAW, 0);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_arg_init_from_buf(&arg, &buf, 0, VACCEL_ARG_RAW,
					       0);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_arg_init_from_buf(&arg, nullptr, sizeof(buf),
					       VACCEL_ARG_RAW, 0);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	ret = vaccel_arg_init_from_buf(&arg, &buf, sizeof(buf), VACCEL_ARG_RAW,
				       0);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(*(int32_t *)arg.buf == 1);
	REQUIRE(arg.size == sizeof(buf));
	REQUIRE(arg.type == VACCEL_ARG_RAW);
	REQUIRE(arg.custom_type_id == 0);
	REQUIRE(arg.owned == false);

	REQUIRE(vaccel_arg_release(&arg) == VACCEL_OK);
}

TEST_CASE("vaccel_arg_release", "[core][arg]")
{
	int ret;
	int32_t buf = 1;
	struct vaccel_arg arg;

	REQUIRE(vaccel_arg_init(&arg, &buf, sizeof(buf), VACCEL_ARG_RAW, 0) ==
		VACCEL_OK);

	SECTION("invalid arguments")
	{
		ret = vaccel_arg_release(nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(*(int32_t *)arg.buf == 1);
		REQUIRE(arg.size == sizeof(buf));
		REQUIRE(arg.type == VACCEL_ARG_RAW);
		REQUIRE(arg.custom_type_id == 0);
		REQUIRE(arg.owned == true);
	}

	ret = vaccel_arg_release(&arg);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE_FALSE(arg.buf);
	REQUIRE(arg.size == 0);
	REQUIRE(arg.type == VACCEL_ARG_MAX);
	REQUIRE(arg.custom_type_id == 0);
	REQUIRE(arg.owned == false);
}

TEST_CASE("vaccel_arg_new", "[core][arg]")
{
	int ret;
	int32_t buf = 1;
	struct vaccel_arg *arg;

	SECTION("invalid arguments")
	{
		ret = vaccel_arg_new(nullptr, &buf, sizeof(buf), VACCEL_ARG_RAW,
				     0);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_arg_new(&arg, &buf, 0, VACCEL_ARG_RAW, 0);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	ret = vaccel_arg_new(&arg, &buf, sizeof(buf), VACCEL_ARG_RAW, 0);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(*(int32_t *)arg->buf == 1);
	REQUIRE(arg->size == sizeof(buf));
	REQUIRE(arg->type == VACCEL_ARG_RAW);
	REQUIRE(arg->custom_type_id == 0);
	REQUIRE(arg->owned == true);

	REQUIRE(vaccel_arg_delete(arg) == VACCEL_OK);
}

TEST_CASE("vaccel_arg_from_buf", "[core][arg]")
{
	int ret;
	int32_t buf = 1;
	struct vaccel_arg *arg;

	SECTION("invalid arguments")
	{
		ret = vaccel_arg_from_buf(nullptr, &buf, sizeof(buf),
					  VACCEL_ARG_RAW, 0);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_arg_from_buf(&arg, nullptr, sizeof(buf),
					  VACCEL_ARG_RAW, 0);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_arg_from_buf(&arg, &buf, 0, VACCEL_ARG_RAW, 0);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	ret = vaccel_arg_from_buf(&arg, &buf, sizeof(buf), VACCEL_ARG_RAW, 0);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(*(int32_t *)arg->buf == 1);
	REQUIRE(arg->size == sizeof(buf));
	REQUIRE(arg->type == VACCEL_ARG_RAW);
	REQUIRE(arg->custom_type_id == 0);
	REQUIRE(arg->owned == false);

	REQUIRE(vaccel_arg_delete(arg) == VACCEL_OK);
}

TEST_CASE("vaccel_arg_delete", "[core][arg]")
{
	int ret;
	int32_t buf = 1;
	struct vaccel_arg *arg;

	REQUIRE(vaccel_arg_new(&arg, &buf, sizeof(buf), VACCEL_ARG_RAW, 0) ==
		VACCEL_OK);

	SECTION("invalid arguments")
	{
		ret = vaccel_arg_delete(nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(*(int32_t *)arg->buf == 1);
		REQUIRE(arg->size == sizeof(buf));
		REQUIRE(arg->type == VACCEL_ARG_RAW);
		REQUIRE(arg->custom_type_id == 0);
		REQUIRE(arg->owned == true);
	}

	ret = vaccel_arg_delete(arg);
	REQUIRE(ret == VACCEL_OK);
}

TEST_CASE("vaccel_arg_array_init", "[core][arg]")
{
	int ret;
	struct vaccel_arg_array args;

	SECTION("invalid arguments")
	{
		ret = vaccel_arg_array_init(nullptr, 1);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	SECTION("default capacity")
	{
		ret = vaccel_arg_array_init(&args, 0);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(args.args);
		REQUIRE(args.count == 0);
		REQUIRE(args.capacity == ARG_ARRAY_CAPACITY_DEFAULT);
		REQUIRE(args.position == 0);
		REQUIRE(args.owned == true);

		REQUIRE(vaccel_arg_array_release(&args) == VACCEL_OK);
	}

	ret = vaccel_arg_array_init(&args, 1);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(args.args);
	REQUIRE(args.count == 0);
	REQUIRE(args.capacity == 1);
	REQUIRE(args.position == 0);
	REQUIRE(args.owned == true);

	REQUIRE(vaccel_arg_array_release(&args) == VACCEL_OK);
}

TEST_CASE("vaccel_arg_array_release", "[core][arg]")
{
	int ret;
	struct vaccel_arg_array args;

	REQUIRE(vaccel_arg_array_init(&args, 1) == VACCEL_OK);

	SECTION("invalid arguments")
	{
		ret = vaccel_arg_array_release(nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.args);
		REQUIRE(args.count == 0);
		REQUIRE(args.capacity == 1);
		REQUIRE(args.position == 0);
		REQUIRE(args.owned == true);
	}

	ret = vaccel_arg_array_release(&args);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE_FALSE(args.args);
	REQUIRE(args.count == 0);
	REQUIRE(args.capacity == 0);
	REQUIRE(args.position == 0);
	REQUIRE(args.owned == false);
}

TEST_CASE("vaccel_arg_array_new", "[core][arg]")
{
	int ret;
	struct vaccel_arg_array *args;

	SECTION("invalid arguments")
	{
		ret = vaccel_arg_array_new(nullptr, 1);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	ret = vaccel_arg_array_new(&args, 1);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(args->args);
	REQUIRE(args->count == 0);
	REQUIRE(args->capacity == 1);
	REQUIRE(args->position == 0);
	REQUIRE(args->owned == true);

	REQUIRE(vaccel_arg_array_delete(args) == VACCEL_OK);
}

TEST_CASE("vaccel_arg_array_delete", "[core][arg]")
{
	int ret;
	struct vaccel_arg_array *args;

	REQUIRE(vaccel_arg_array_new(&args, 1) == VACCEL_OK);

	SECTION("invalid arguments")
	{
		ret = vaccel_arg_array_delete(nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args->args);
		REQUIRE(args->count == 0);
		REQUIRE(args->capacity == 1);
		REQUIRE(args->position == 0);
		REQUIRE(args->owned == true);
	}

	ret = vaccel_arg_array_delete(args);
	REQUIRE(ret == VACCEL_OK);
}

TEST_CASE("vaccel_arg_array_wrap", "[core][arg]")
{
	int ret;
	int32_t buf = 1;
	struct vaccel_arg raw_args[2];
	struct vaccel_arg_array args;

	REQUIRE(vaccel_arg_init(&raw_args[0], &buf, sizeof(buf), VACCEL_ARG_RAW,
				0) == VACCEL_OK);
	REQUIRE(vaccel_arg_init(&raw_args[1], &buf, sizeof(buf), VACCEL_ARG_RAW,
				0) == VACCEL_OK);

	SECTION("invalid arguments")
	{
		ret = vaccel_arg_array_wrap(nullptr, raw_args, 1);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_arg_array_wrap(&args, nullptr, 1);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	ret = vaccel_arg_array_wrap(&args, raw_args, 2);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(args.args == raw_args);
	REQUIRE(args.count == 2);
	REQUIRE(args.capacity == args.count);
	REQUIRE(args.position == 0);
	REQUIRE(args.owned == false);

	REQUIRE(vaccel_arg_array_release(&args) == VACCEL_OK);
	REQUIRE(vaccel_arg_release(&raw_args[0]) == VACCEL_OK);
	REQUIRE(vaccel_arg_release(&raw_args[1]) == VACCEL_OK);
}

TEST_CASE("vaccel_arg_array_clear", "[core][arg]")
{
	int32_t buf = 1;
	int32_t tmp_buf = 1;
	struct vaccel_arg_array args;

	REQUIRE(vaccel_arg_array_init(&args, 1) == VACCEL_OK);
	REQUIRE(vaccel_arg_array_add_int32(&args, &buf) == VACCEL_OK);
	REQUIRE(vaccel_arg_array_get_int32(&args, &tmp_buf) == VACCEL_OK);

	SECTION("invalid arguments")
	{
		vaccel_arg_array_clear(nullptr);
		REQUIRE(args.args);
		REQUIRE(*(int32_t *)args.args[0].buf == 1);
		REQUIRE(args.count == 1);
		REQUIRE(args.position == 1);

		struct vaccel_arg *tmp = args.args;
		args.args = nullptr;

		vaccel_arg_array_clear(&args);
		REQUIRE_FALSE(args.args);
		REQUIRE(args.count == 1);
		REQUIRE(args.position == 1);

		args.args = tmp;
	}

	vaccel_arg_array_clear(&args);
	REQUIRE(args.args);
	REQUIRE(args.args[0].buf == nullptr);
	REQUIRE(args.count == 0);
	REQUIRE(args.position == 0);

	REQUIRE(vaccel_arg_array_release(&args) == VACCEL_OK);
}

TEST_CASE("vaccel_arg_array_add_raw", "[core][arg]")
{
	int ret;
	int32_t buf = 1;
	struct vaccel_arg_array args;

	REQUIRE(vaccel_arg_array_init(&args, 1) == VACCEL_OK);

	ret = vaccel_arg_array_add_raw(&args, &buf, sizeof(buf));
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(args.count == 1);
	REQUIRE(*(int32_t *)args.args[0].buf == buf);
	REQUIRE(args.args[0].size == sizeof(buf));
	REQUIRE(args.args[0].type == VACCEL_ARG_RAW);
	REQUIRE(args.args[0].custom_type_id == 0);
	REQUIRE(args.args[0].owned == false);

	SECTION("grow capacity")
	{
		ret = vaccel_arg_array_add_raw(&args, &buf, sizeof(buf));
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(args.count == 2);
		REQUIRE(*(int32_t *)args.args[1].buf == 1);
		REQUIRE(args.args[1].size == sizeof(buf));
		REQUIRE(args.args[1].type == VACCEL_ARG_RAW);
		REQUIRE(args.args[1].custom_type_id == 0);
		REQUIRE(args.args[1].owned == false);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_arg_array_add_raw(nullptr, &buf, sizeof(buf));
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_arg_array_add_raw(&args, nullptr, sizeof(buf));
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_arg_array_add_raw(&args, &buf, 0);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	REQUIRE(vaccel_arg_array_release(&args) == VACCEL_OK);
}

#define ARG_ARRAY_DEFINE_TEST_ADD_FUNCS(TYPE_NAME, C_TYPE, ARG_TYPE,           \
					ARRAY_TYPE)                            \
	TEST_CASE("vaccel_arg_array_add_" #TYPE_NAME, "[core][arg]")           \
	{                                                                      \
		int ret;                                                       \
		C_TYPE buf = 1;                                                \
		struct vaccel_arg_array args;                                  \
                                                                               \
		REQUIRE(vaccel_arg_array_init(&args, 1) == VACCEL_OK);         \
                                                                               \
		ret = vaccel_arg_array_add_##TYPE_NAME(&args, &buf);           \
		REQUIRE(ret == VACCEL_OK);                                     \
		REQUIRE(args.count == 1);                                      \
		REQUIRE(*(C_TYPE *)args.args[0].buf == buf);                   \
		REQUIRE(args.args[0].size == sizeof(buf));                     \
		REQUIRE(args.args[0].type == ARG_TYPE);                        \
		REQUIRE(args.args[0].custom_type_id == 0);                     \
		REQUIRE(args.args[0].owned == false);                          \
                                                                               \
		SECTION("grow capacity")                                       \
		{                                                              \
			ret = vaccel_arg_array_add_##TYPE_NAME(&args, &buf);   \
			REQUIRE(ret == VACCEL_OK);                             \
			REQUIRE(args.count == 2);                              \
			REQUIRE(*(C_TYPE *)args.args[1].buf == buf);           \
			REQUIRE(args.args[1].size == sizeof(buf));             \
			REQUIRE(args.args[1].type == ARG_TYPE);                \
			REQUIRE(args.args[1].custom_type_id == 0);             \
			REQUIRE(args.args[1].owned == false);                  \
		}                                                              \
                                                                               \
		SECTION("invalid arguments")                                   \
		{                                                              \
			ret = vaccel_arg_array_add_##TYPE_NAME(nullptr, &buf); \
			REQUIRE(ret == VACCEL_EINVAL);                         \
                                                                               \
			ret = vaccel_arg_array_add_##TYPE_NAME(&args,          \
							       nullptr);       \
			REQUIRE(ret == VACCEL_EINVAL);                         \
		}                                                              \
                                                                               \
		REQUIRE(vaccel_arg_array_release(&args) == VACCEL_OK);         \
	}                                                                      \
                                                                               \
	TEST_CASE("vaccel_arg_array_add_" #TYPE_NAME "_array", "[core][arg]")  \
	{                                                                      \
		int ret;                                                       \
		const size_t buf_count = 4;                                    \
		C_TYPE buf[buf_count] = { 1 };                                 \
		struct vaccel_arg_array args;                                  \
                                                                               \
		REQUIRE(vaccel_arg_array_init(&args, 1) == VACCEL_OK);         \
                                                                               \
		ret = vaccel_arg_array_add_##TYPE_NAME##_array(&args, buf,     \
							       buf_count);     \
		REQUIRE(ret == VACCEL_OK);                                     \
		REQUIRE(args.count == 1);                                      \
		REQUIRE(args.args[0].buf == buf);                              \
		REQUIRE(args.args[0].size == sizeof(buf));                     \
		REQUIRE(args.args[0].type == ARRAY_TYPE);                      \
		REQUIRE(args.args[0].custom_type_id == 0);                     \
		REQUIRE(args.args[0].owned == false);                          \
                                                                               \
		SECTION("grow capacity")                                       \
		{                                                              \
			ret = vaccel_arg_array_add_##TYPE_NAME##_array(        \
				&args, buf, buf_count);                        \
			REQUIRE(ret == VACCEL_OK);                             \
			REQUIRE(args.count == 2);                              \
			REQUIRE(args.args[1].buf == buf);                      \
			REQUIRE(args.args[1].size == sizeof(buf));             \
			REQUIRE(args.args[1].type == ARRAY_TYPE);              \
			REQUIRE(args.args[1].custom_type_id == 0);             \
			REQUIRE(args.args[1].owned == false);                  \
		}                                                              \
                                                                               \
		SECTION("invalid arguments")                                   \
		{                                                              \
			ret = vaccel_arg_array_add_##TYPE_NAME##_array(        \
				nullptr, buf, buf_count);                      \
			REQUIRE(ret == VACCEL_EINVAL);                         \
                                                                               \
			ret = vaccel_arg_array_add_##TYPE_NAME##_array(        \
				&args, nullptr, buf_count);                    \
			REQUIRE(ret == VACCEL_EINVAL);                         \
                                                                               \
			ret = vaccel_arg_array_add_##TYPE_NAME##_array(        \
				&args, buf, 0);                                \
			REQUIRE(ret == VACCEL_EINVAL);                         \
		}                                                              \
                                                                               \
		REQUIRE(vaccel_arg_array_release(&args) == VACCEL_OK);         \
	}

VACCEL_ARG_NUMERIC_TYPES(ARG_ARRAY_DEFINE_TEST_ADD_FUNCS)

TEST_CASE("vaccel_arg_array_add_string", "[core][arg]")
{
	int ret;
	char buf[] = "test";
	struct vaccel_arg_array args;

	REQUIRE(vaccel_arg_array_init(&args, 1) == VACCEL_OK);

	ret = vaccel_arg_array_add_string(&args, buf);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(args.count == 1);
	REQUIRE(strcmp((const char *)args.args[0].buf, buf) == 0);
	REQUIRE(args.args[0].size == strlen(buf) + 1);
	REQUIRE(args.args[0].type == VACCEL_ARG_STRING);
	REQUIRE(args.args[0].custom_type_id == 0);
	REQUIRE(args.args[0].owned == false);

	SECTION("grow capacity")
	{
		ret = vaccel_arg_array_add_string(&args, buf);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(args.count == 2);
		REQUIRE(strcmp((const char *)args.args[1].buf, buf) == 0);
		REQUIRE(args.args[1].size == strlen(buf) + 1);
		REQUIRE(args.args[1].type == VACCEL_ARG_STRING);
		REQUIRE(args.args[1].custom_type_id == 0);
		REQUIRE(args.args[1].owned == false);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_arg_array_add_string(nullptr, buf);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_arg_array_add_string(&args, nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	REQUIRE(vaccel_arg_array_release(&args) == VACCEL_OK);
}

TEST_CASE("vaccel_arg_array_add_buffer", "[core][arg]")
{
	int ret;
	char buf[] = "test";
	const size_t size = strlen(buf) + 1;
	struct vaccel_arg_array args;

	REQUIRE(vaccel_arg_array_init(&args, 1) == VACCEL_OK);

	ret = vaccel_arg_array_add_buffer(&args, buf, size);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(args.count == 1);
	REQUIRE(strcmp((const char *)args.args[0].buf, buf) == 0);
	REQUIRE(args.args[0].size == size);
	REQUIRE(args.args[0].type == VACCEL_ARG_BUFFER);
	REQUIRE(args.args[0].custom_type_id == 0);
	REQUIRE(args.args[0].owned == false);

	SECTION("grow capacity")
	{
		ret = vaccel_arg_array_add_buffer(&args, buf, size);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(args.count == 2);
		REQUIRE(strcmp((const char *)args.args[1].buf, buf) == 0);
		REQUIRE(args.args[1].size == size);
		REQUIRE(args.args[1].type == VACCEL_ARG_BUFFER);
		REQUIRE(args.args[1].custom_type_id == 0);
		REQUIRE(args.args[1].owned == false);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_arg_array_add_buffer(nullptr, buf, size);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_arg_array_add_buffer(&args, nullptr, size);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_arg_array_add_buffer(&args, buf, 0);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	REQUIRE(vaccel_arg_array_release(&args) == VACCEL_OK);
}

enum { TEST_ARG_TYPE_ID = 1 };
auto validate_arg_type(const void *buf, size_t size, uint32_t custom_id) -> bool
{
	if (buf == nullptr || size != sizeof(vaccel_arg_type_t))
		return false;
	if (custom_id != TEST_ARG_TYPE_ID)
		return false;

	const vaccel_arg_type_t value = *(const vaccel_arg_type_t *)buf;
	return value < VACCEL_ARG_MAX;
}

TEST_CASE("vaccel_arg_array_add_custom", "[core][arg]")
{
	int ret;
	vaccel_arg_type_t buf = VACCEL_ARG_RAW;
	const size_t size = sizeof(buf);
	struct vaccel_arg_array args;

	REQUIRE(vaccel_arg_array_init(&args, 1) == VACCEL_OK);

	ret = vaccel_arg_array_add_custom(&args, TEST_ARG_TYPE_ID, (void *)&buf,
					  size, validate_arg_type);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(args.count == 1);
	REQUIRE(*(vaccel_arg_type_t *)args.args[0].buf == VACCEL_ARG_RAW);
	REQUIRE(args.args[0].size == size);
	REQUIRE(args.args[0].type == VACCEL_ARG_CUSTOM);
	REQUIRE(args.args[0].custom_type_id == TEST_ARG_TYPE_ID);
	REQUIRE(args.args[0].owned == false);

	SECTION("grow capacity")
	{
		ret = vaccel_arg_array_add_custom(&args, TEST_ARG_TYPE_ID,
						  (void *)&buf, size,
						  validate_arg_type);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(args.count == 2);
		REQUIRE(*(vaccel_arg_type_t *)args.args[1].buf ==
			VACCEL_ARG_RAW);
		REQUIRE(args.args[1].size == size);
		REQUIRE(args.args[1].type == VACCEL_ARG_CUSTOM);
		REQUIRE(args.args[1].custom_type_id == TEST_ARG_TYPE_ID);
		REQUIRE(args.args[1].owned == false);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_arg_array_add_custom(nullptr, TEST_ARG_TYPE_ID,
						  (void *)&buf, size,
						  validate_arg_type);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_arg_array_add_custom(&args, TEST_ARG_TYPE_ID,
						  (void *)&buf, size, nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	SECTION("invalid validator arguments")
	{
		ret = vaccel_arg_array_add_custom(&args, TEST_ARG_TYPE_ID,
						  nullptr, size,
						  validate_arg_type);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_arg_array_add_custom(&args, TEST_ARG_TYPE_ID,
						  (void *)&buf, 0,
						  validate_arg_type);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	REQUIRE(vaccel_arg_array_release(&args) == VACCEL_OK);
}

TEST_CASE("vaccel_arg_array_add_serialized", "[core][arg]")
{
	int ret;
	struct mydata buf;
	struct mydata deser_buf;
	const size_t size = sizeof(buf);
	const size_t mydata_count = 6;
	struct vaccel_arg_array args;

	buf.count = mydata_count;
	buf.array = (uint32_t *)malloc(buf.count * sizeof(*buf.array));
	REQUIRE(buf.array);

	for (uint32_t i = 0; i < buf.count; i++)
		buf.array[i] = 2 * i;

	REQUIRE(vaccel_arg_array_init(&args, 1) == VACCEL_OK);

	ret = vaccel_arg_array_add_serialized(&args, VACCEL_ARG_CUSTOM,
					      MYDATA_TYPE_ID, &buf, size,
					      mydata_serialize);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(args.count == 1);
	REQUIRE(args.args[0].buf);
	REQUIRE(args.args[0].size);
	REQUIRE(args.args[0].type == VACCEL_ARG_CUSTOM);
	REQUIRE(args.args[0].custom_type_id == MYDATA_TYPE_ID);
	REQUIRE(args.args[0].owned == true);

	// verify data
	REQUIRE(vaccel_arg_array_get_serialized(
			&args, VACCEL_ARG_CUSTOM, MYDATA_TYPE_ID, &deser_buf,
			size, mydata_deserialize) == VACCEL_OK);
	REQUIRE(deser_buf.count == buf.count);
	for (uint32_t i = 0; i < buf.count; i++)
		REQUIRE(deser_buf.array[i] == buf.array[i]);

	SECTION("grow capacity")
	{
		ret = vaccel_arg_array_add_serialized(&args, VACCEL_ARG_CUSTOM,
						      MYDATA_TYPE_ID, &buf,
						      size, mydata_serialize);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(args.count == 2);
		REQUIRE(args.args[1].buf);
		REQUIRE(args.args[1].size);
		REQUIRE(args.args[1].type == VACCEL_ARG_CUSTOM);
		REQUIRE(args.args[1].custom_type_id == MYDATA_TYPE_ID);
		REQUIRE(args.args[1].owned == true);
	}

	SECTION("invalid arguments")
	{
		ret = vaccel_arg_array_add_serialized(nullptr,
						      VACCEL_ARG_CUSTOM,
						      MYDATA_TYPE_ID, &buf,
						      size, mydata_serialize);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_arg_array_add_serialized(&args, VACCEL_ARG_CUSTOM,
						      MYDATA_TYPE_ID, &buf,
						      size, nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	SECTION("invalid serializer arguments")
	{
		ret = vaccel_arg_array_add_serialized(&args, VACCEL_ARG_CUSTOM,
						      MYDATA_TYPE_ID, nullptr,
						      size, mydata_serialize);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_arg_array_add_serialized(&args, VACCEL_ARG_CUSTOM,
						      MYDATA_TYPE_ID, &buf, 0,
						      mydata_serialize);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	REQUIRE(vaccel_arg_array_release(&args) == VACCEL_OK);
	free(buf.array);
	free(deser_buf.array);
}

TEST_CASE("vaccel_arg_array_add_range", "[core][arg]")
{
	int ret;
	int32_t buf = 1;
	const size_t arg_count = 6;
	struct vaccel_arg_array args;
	struct vaccel_arg_array dup_args;

	REQUIRE(vaccel_arg_array_init(&args, arg_count) == VACCEL_OK);
	for (size_t i = 0; i < arg_count; i++)
		REQUIRE(vaccel_arg_array_add_int32(&args, &buf) == VACCEL_OK);

	SECTION("no args")
	{
		REQUIRE(vaccel_arg_array_init(&dup_args, arg_count) ==
			VACCEL_OK);

		ret = vaccel_arg_array_add_range(&dup_args, &args, 1, 0, false);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(dup_args.count == 0);

		REQUIRE(vaccel_arg_array_release(&dup_args) == VACCEL_OK);
	}

	SECTION("reference arg bufs")
	{
		REQUIRE(vaccel_arg_array_init(&dup_args, arg_count) ==
			VACCEL_OK);

		ret = vaccel_arg_array_add_range(&dup_args, &args, 1, 3, false);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(dup_args.count == 3);
		for (size_t i = 0; i < dup_args.count; i++) {
			REQUIRE(dup_args.args[i].buf == args.args[i].buf);
			REQUIRE(dup_args.args[i].size == args.args[i].size);
			REQUIRE(dup_args.args[i].type == args.args[i].type);
			REQUIRE(dup_args.args[i].custom_type_id ==
				args.args[i].custom_type_id);
			REQUIRE(dup_args.args[i].owned == false);
		}

		REQUIRE(vaccel_arg_array_release(&dup_args) == VACCEL_OK);
	}

	SECTION("grow array")
	{
		REQUIRE(vaccel_arg_array_init(&dup_args, 3) == VACCEL_OK);

		ret = vaccel_arg_array_add_range(&dup_args, &args, 1, 3, false);
		REQUIRE(ret == VACCEL_OK);
		ret = vaccel_arg_array_add_range(&dup_args, &args, 1, 3, false);
		REQUIRE(ret == VACCEL_OK);

		REQUIRE(dup_args.count == 6);
		for (size_t i = 0; i < dup_args.count / 2; i++) {
			REQUIRE(dup_args.args[i].buf == args.args[i].buf);
			REQUIRE(dup_args.args[i].size == args.args[i].size);
			REQUIRE(dup_args.args[i].type == args.args[i].type);
			REQUIRE(dup_args.args[i].custom_type_id ==
				args.args[i].custom_type_id);
			REQUIRE(dup_args.args[i].owned == false);
		}
		for (size_t i = dup_args.count / 2; i < dup_args.count; i++) {
			REQUIRE(dup_args.args[i].buf ==
				args.args[i - dup_args.count / 2].buf);
			REQUIRE(dup_args.args[i].size ==
				args.args[i - dup_args.count / 2].size);
			REQUIRE(dup_args.args[i].type ==
				args.args[i - dup_args.count / 2].type);
			REQUIRE(dup_args.args[i].custom_type_id ==
				args.args[i - dup_args.count / 2]
					.custom_type_id);
			REQUIRE(dup_args.args[i].owned == false);
		}

		REQUIRE(vaccel_arg_array_release(&dup_args) == VACCEL_OK);
	}

	SECTION("copy arg bufs")
	{
		REQUIRE(vaccel_arg_array_init(&dup_args, arg_count) ==
			VACCEL_OK);

		ret = vaccel_arg_array_add_range(&dup_args, &args, 2, 4, true);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(dup_args.count == 4);
		for (size_t i = 0; i < dup_args.count; i++) {
			REQUIRE(dup_args.args[i].buf != args.args[i].buf);
			REQUIRE(*(int32_t *)dup_args.args[i].buf ==
				*(int32_t *)args.args[i].buf);
			REQUIRE(dup_args.args[i].size == args.args[i].size);
			REQUIRE(dup_args.args[i].type == args.args[i].type);
			REQUIRE(dup_args.args[i].custom_type_id ==
				args.args[i].custom_type_id);
			REQUIRE(dup_args.args[i].owned == true);
		}

		REQUIRE(vaccel_arg_array_release(&dup_args) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		REQUIRE(vaccel_arg_array_init(&dup_args, arg_count) ==
			VACCEL_OK);

		ret = vaccel_arg_array_add_range(nullptr, &args, 1, 3, false);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_arg_array_add_range(&dup_args, nullptr, 1, 3,
						 false);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_arg_array_add_range(&dup_args, &args, 7, 3, false);
		REQUIRE(ret == VACCEL_ERANGE);

		ret = vaccel_arg_array_add_range(&dup_args, &args, 1, 7, false);
		REQUIRE(ret == VACCEL_ERANGE);

		REQUIRE(vaccel_arg_array_release(&dup_args) == VACCEL_OK);
	}

	REQUIRE(vaccel_arg_array_release(&args) == VACCEL_OK);
}

TEST_CASE("vaccel_arg_array_add_remaining", "[core][arg]")
{
	int ret;
	int32_t buf = 1;
	const size_t arg_count = 6;
	struct vaccel_arg_array args;
	struct vaccel_arg_array dup_args;

	REQUIRE(vaccel_arg_array_init(&args, arg_count) == VACCEL_OK);
	for (size_t i = 0; i < arg_count; i++)
		REQUIRE(vaccel_arg_array_add_int32(&args, &buf) == VACCEL_OK);

	// read 1 arg (arg_count - 1 remaining)
	int32_t tmp_buf;
	REQUIRE(vaccel_arg_array_get_int32(&args, &tmp_buf) == VACCEL_OK);

	SECTION("reference arg bufs")
	{
		REQUIRE(vaccel_arg_array_init(&dup_args, arg_count) ==
			VACCEL_OK);

		ret = vaccel_arg_array_add_remaining(&dup_args, &args, false);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(dup_args.count == args.count - 1);
		for (size_t i = 0; i < dup_args.count; i++) {
			REQUIRE(dup_args.args[i].buf ==
				args.args[args.position + i].buf);
			REQUIRE(dup_args.args[i].size ==
				args.args[args.position + i].size);
			REQUIRE(dup_args.args[i].type ==
				args.args[args.position + i].type);
			REQUIRE(dup_args.args[i].custom_type_id ==
				args.args[args.position + i].custom_type_id);
			REQUIRE(dup_args.args[i].owned == false);
		}

		REQUIRE(vaccel_arg_array_release(&dup_args) == VACCEL_OK);
	}

	SECTION("copy arg bufs")
	{
		REQUIRE(vaccel_arg_array_init(&dup_args, arg_count) ==
			VACCEL_OK);

		ret = vaccel_arg_array_add_remaining(&dup_args, &args, true);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(dup_args.count == args.count - 1);
		for (size_t i = 0; i < dup_args.count; i++) {
			REQUIRE(dup_args.args[i].buf !=
				args.args[args.position + i].buf);
			REQUIRE(*(int32_t *)dup_args.args[i].buf ==
				*(int32_t *)args.args[args.position + i].buf);
			REQUIRE(dup_args.args[i].size ==
				args.args[args.position + i].size);
			REQUIRE(dup_args.args[i].type ==
				args.args[args.position + i].type);
			REQUIRE(dup_args.args[i].custom_type_id ==
				args.args[args.position + i].custom_type_id);
			REQUIRE(dup_args.args[i].owned == true);
		}

		REQUIRE(vaccel_arg_array_release(&dup_args) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		REQUIRE(vaccel_arg_array_init(&dup_args, arg_count) ==
			VACCEL_OK);

		ret = vaccel_arg_array_add_remaining(nullptr, &args, false);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_arg_array_add_remaining(&dup_args, nullptr, false);
		REQUIRE(ret == VACCEL_EINVAL);

		REQUIRE(vaccel_arg_array_release(&dup_args) == VACCEL_OK);
	}

	// read all args
	for (size_t i = args.position; i < arg_count; i++)
		REQUIRE(vaccel_arg_array_get_int32(&args, &tmp_buf) ==
			VACCEL_OK);

	SECTION("no remaining")
	{
		REQUIRE(vaccel_arg_array_init(&dup_args, arg_count) ==
			VACCEL_OK);

		ret = vaccel_arg_array_add_remaining(&dup_args, &args, false);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(dup_args.count == 0);

		REQUIRE(vaccel_arg_array_release(&dup_args) == VACCEL_OK);
	}

	REQUIRE(vaccel_arg_array_release(&args) == VACCEL_OK);
}

TEST_CASE("vaccel_arg_array_add_all", "[core][arg]")
{
	int ret;
	int32_t buf = 1;
	const size_t arg_count = 6;
	struct vaccel_arg_array args;
	struct vaccel_arg_array dup_args;

	REQUIRE(vaccel_arg_array_init(&args, arg_count) == VACCEL_OK);

	SECTION("no args")
	{
		REQUIRE(vaccel_arg_array_init(&dup_args, arg_count) ==
			VACCEL_OK);

		ret = vaccel_arg_array_add_all(&dup_args, &args, false);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(dup_args.count == 0);

		REQUIRE(vaccel_arg_array_release(&dup_args) == VACCEL_OK);
	}

	for (size_t i = 0; i < arg_count; i++)
		REQUIRE(vaccel_arg_array_add_int32(&args, &buf) == VACCEL_OK);

	SECTION("reference arg bufs")
	{
		REQUIRE(vaccel_arg_array_init(&dup_args, arg_count) ==
			VACCEL_OK);

		ret = vaccel_arg_array_add_all(&dup_args, &args, false);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(dup_args.count == args.count);
		for (size_t i = 0; i < dup_args.count; i++) {
			REQUIRE(dup_args.args[i].buf == args.args[i].buf);
			REQUIRE(dup_args.args[i].size == args.args[i].size);
			REQUIRE(dup_args.args[i].type == args.args[i].type);
			REQUIRE(dup_args.args[i].custom_type_id ==
				args.args[i].custom_type_id);
			REQUIRE(dup_args.args[i].owned == false);
		}

		REQUIRE(vaccel_arg_array_release(&dup_args) == VACCEL_OK);
	}

	SECTION("grow array")
	{
		REQUIRE(vaccel_arg_array_init(&dup_args, arg_count) ==
			VACCEL_OK);

		ret = vaccel_arg_array_add_all(&dup_args, &args, false);
		REQUIRE(ret == VACCEL_OK);
		ret = vaccel_arg_array_add_all(&dup_args, &args, false);
		REQUIRE(ret == VACCEL_OK);

		REQUIRE(dup_args.count == 2 * args.count);
		for (size_t i = 0; i < dup_args.count / 2; i++) {
			REQUIRE(dup_args.args[i].buf == args.args[i].buf);
			REQUIRE(dup_args.args[i].size == args.args[i].size);
			REQUIRE(dup_args.args[i].type == args.args[i].type);
			REQUIRE(dup_args.args[i].custom_type_id ==
				args.args[i].custom_type_id);
			REQUIRE(dup_args.args[i].owned == false);
		}
		for (size_t i = dup_args.count / 2; i < dup_args.count; i++) {
			REQUIRE(dup_args.args[i].buf ==
				args.args[i - dup_args.count / 2].buf);
			REQUIRE(dup_args.args[i].size ==
				args.args[i - dup_args.count / 2].size);
			REQUIRE(dup_args.args[i].type ==
				args.args[i - dup_args.count / 2].type);
			REQUIRE(dup_args.args[i].custom_type_id ==
				args.args[i - dup_args.count / 2]
					.custom_type_id);
			REQUIRE(dup_args.args[i].owned == false);
		}

		REQUIRE(vaccel_arg_array_release(&dup_args) == VACCEL_OK);
	}

	SECTION("copy arg bufs")
	{
		REQUIRE(vaccel_arg_array_init(&dup_args, arg_count) ==
			VACCEL_OK);

		ret = vaccel_arg_array_add_all(&dup_args, &args, true);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(dup_args.count == args.count);
		for (size_t i = 0; i < dup_args.count; i++) {
			REQUIRE(dup_args.args[i].buf != args.args[i].buf);
			REQUIRE(*(int32_t *)dup_args.args[i].buf ==
				*(int32_t *)args.args[i].buf);
			REQUIRE(dup_args.args[i].size == args.args[i].size);
			REQUIRE(dup_args.args[i].type == args.args[i].type);
			REQUIRE(dup_args.args[i].custom_type_id ==
				args.args[i].custom_type_id);
			REQUIRE(dup_args.args[i].owned == true);
		}

		REQUIRE(vaccel_arg_array_release(&dup_args) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		REQUIRE(vaccel_arg_array_init(&dup_args, arg_count) ==
			VACCEL_OK);

		ret = vaccel_arg_array_add_all(nullptr, &args, false);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_arg_array_add_all(&dup_args, nullptr, false);
		REQUIRE(ret == VACCEL_EINVAL);

		REQUIRE(vaccel_arg_array_release(&dup_args) == VACCEL_OK);
	}

	REQUIRE(vaccel_arg_array_release(&args) == VACCEL_OK);
}

TEST_CASE("vaccel_arg_array_get_raw", "[core][arg]")
{
	int ret;
	int32_t buf = 1;
	void *get_buf;
	size_t get_size;
	struct vaccel_arg_array args;

	REQUIRE(vaccel_arg_array_init(&args, 1) == VACCEL_OK);
	REQUIRE(vaccel_arg_array_add_raw(&args, &buf, sizeof(buf)) ==
		VACCEL_OK);

	SECTION("invalid arguments")
	{
		ret = vaccel_arg_array_get_raw(nullptr, &get_buf, &get_size);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 0);

		ret = vaccel_arg_array_get_raw(&args, nullptr, &get_size);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 0);

		ret = vaccel_arg_array_get_raw(&args, &get_buf, nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 0);
	}

	ret = vaccel_arg_array_get_raw(&args, &get_buf, &get_size);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(args.position == 1);
	REQUIRE(*(int32_t *)get_buf == buf);
	REQUIRE(get_size == sizeof(buf));

	SECTION("out of range")
	{
		ret = vaccel_arg_array_get_raw(&args, &get_buf, &get_size);
		REQUIRE(ret == VACCEL_ERANGE);
		REQUIRE(args.position == 1);
	}

	SECTION("invalid type")
	{
		REQUIRE(vaccel_arg_array_add_buffer(&args, &buf, sizeof(buf)) ==
			VACCEL_OK);

		ret = vaccel_arg_array_get_raw(&args, &get_buf, &get_size);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 1);

		void *tmp;
		size_t tmp_size;
		REQUIRE(vaccel_arg_array_get_buffer(&args, &tmp, &tmp_size) ==
			VACCEL_OK);
	}

	REQUIRE(vaccel_arg_array_release(&args) == VACCEL_OK);
}

#define ARG_ARRAY_DEFINE_TEST_GET_FUNCS(TYPE_NAME, C_TYPE, ARG_TYPE,           \
					ARRAY_TYPE)                            \
	TEST_CASE("vaccel_arg_array_get_" #TYPE_NAME, "[core][arg]")           \
	{                                                                      \
		int ret;                                                       \
		C_TYPE buf = 1;                                                \
		C_TYPE get_buf;                                                \
		struct vaccel_arg_array args;                                  \
                                                                               \
		REQUIRE(vaccel_arg_array_init(&args, 1) == VACCEL_OK);         \
		REQUIRE(vaccel_arg_array_add_##TYPE_NAME(&args, &buf) ==       \
			VACCEL_OK);                                            \
                                                                               \
		SECTION("invalid arguments")                                   \
		{                                                              \
			ret = vaccel_arg_array_get_##TYPE_NAME(nullptr,        \
							       &get_buf);      \
			REQUIRE(ret == VACCEL_EINVAL);                         \
			REQUIRE(args.position == 0);                           \
                                                                               \
			ret = vaccel_arg_array_get_##TYPE_NAME(&args,          \
							       nullptr);       \
			REQUIRE(ret == VACCEL_EINVAL);                         \
			REQUIRE(args.position == 0);                           \
		}                                                              \
                                                                               \
		ret = vaccel_arg_array_get_##TYPE_NAME(&args, &get_buf);       \
		REQUIRE(ret == VACCEL_OK);                                     \
		REQUIRE(args.position == 1);                                   \
		REQUIRE(get_buf == buf);                                       \
                                                                               \
		SECTION("out of range")                                        \
		{                                                              \
			ret = vaccel_arg_array_get_##TYPE_NAME(&args,          \
							       &get_buf);      \
			REQUIRE(ret == VACCEL_ERANGE);                         \
			REQUIRE(args.position == 1);                           \
		}                                                              \
                                                                               \
		SECTION("invalid type")                                        \
		{                                                              \
			REQUIRE(vaccel_arg_array_add_buffer(&args, &buf,       \
							    sizeof(buf)) ==    \
				VACCEL_OK);                                    \
                                                                               \
			ret = vaccel_arg_array_get_##TYPE_NAME(&args,          \
							       &get_buf);      \
			REQUIRE(ret == VACCEL_EINVAL);                         \
			REQUIRE(args.position == 1);                           \
                                                                               \
			void *tmp;                                             \
			size_t tmp_size;                                       \
			REQUIRE(vaccel_arg_array_get_buffer(                   \
					&args, &tmp, &tmp_size) == VACCEL_OK); \
		}                                                              \
                                                                               \
		SECTION("invalid buf")                                         \
		{                                                              \
			REQUIRE(vaccel_arg_array_add_##TYPE_NAME(              \
					&args, &buf) == VACCEL_OK);            \
                                                                               \
			void *tmp = args.args[args.position].buf;              \
			args.args[args.position].buf = nullptr;                \
                                                                               \
			ret = vaccel_arg_array_get_##TYPE_NAME(&args,          \
							       &get_buf);      \
			REQUIRE(ret == VACCEL_EINVAL);                         \
			REQUIRE(args.position == 1);                           \
                                                                               \
			args.args[args.position].buf = tmp;                    \
			const size_t tmp_size = args.args[args.position].size; \
			args.args[args.position].size = 0;                     \
                                                                               \
			ret = vaccel_arg_array_get_##TYPE_NAME(&args,          \
							       &get_buf);      \
			REQUIRE(ret == VACCEL_EINVAL);                         \
			REQUIRE(args.position == 1);                           \
                                                                               \
			args.args[args.position].size = tmp_size;              \
			REQUIRE(vaccel_arg_array_get_##TYPE_NAME(              \
					&args, &get_buf) == VACCEL_OK);        \
		}                                                              \
                                                                               \
		REQUIRE(vaccel_arg_array_release(&args) == VACCEL_OK);         \
	}                                                                      \
                                                                               \
	TEST_CASE("vaccel_arg_array_get_" #TYPE_NAME "_array", "[core][arg]")  \
	{                                                                      \
		int ret;                                                       \
		const size_t buf_count = 4;                                    \
		C_TYPE buf[buf_count] = { 1 };                                 \
		C_TYPE *get_buf;                                               \
		size_t get_count;                                              \
		struct vaccel_arg_array args;                                  \
                                                                               \
		REQUIRE(vaccel_arg_array_init(&args, 1) == VACCEL_OK);         \
		REQUIRE(vaccel_arg_array_add_##TYPE_NAME##_array(              \
				&args, buf, buf_count) == VACCEL_OK);          \
                                                                               \
		SECTION("invalid arguments")                                   \
		{                                                              \
			ret = vaccel_arg_array_get_##TYPE_NAME##_array(        \
				nullptr, &get_buf, &get_count);                \
			REQUIRE(ret == VACCEL_EINVAL);                         \
			REQUIRE(args.position == 0);                           \
                                                                               \
			ret = vaccel_arg_array_get_##TYPE_NAME##_array(        \
				&args, nullptr, &get_count);                   \
			REQUIRE(ret == VACCEL_EINVAL);                         \
			REQUIRE(args.position == 0);                           \
		}                                                              \
                                                                               \
		ret = vaccel_arg_array_get_##TYPE_NAME##_array(                \
			&args, &get_buf, &get_count);                          \
		REQUIRE(ret == VACCEL_OK);                                     \
		REQUIRE(args.position == 1);                                   \
		REQUIRE(get_buf == buf);                                       \
		REQUIRE(get_count == buf_count);                               \
                                                                               \
		SECTION("out of range")                                        \
		{                                                              \
			ret = vaccel_arg_array_get_##TYPE_NAME##_array(        \
				&args, &get_buf, &get_count);                  \
			REQUIRE(ret == VACCEL_ERANGE);                         \
			REQUIRE(args.position == 1);                           \
		}                                                              \
                                                                               \
		SECTION("no count")                                            \
		{                                                              \
			REQUIRE(vaccel_arg_array_add_##TYPE_NAME##_array(      \
					&args, buf, buf_count) == VACCEL_OK);  \
                                                                               \
			ret = vaccel_arg_array_get_##TYPE_NAME##_array(        \
				&args, &get_buf, nullptr);                     \
			REQUIRE(ret == VACCEL_OK);                             \
			REQUIRE(args.position == 2);                           \
			REQUIRE(get_buf == buf);                               \
		}                                                              \
                                                                               \
		SECTION("invalid type")                                        \
		{                                                              \
			REQUIRE(vaccel_arg_array_add_buffer(&args, &buf,       \
							    sizeof(buf)) ==    \
				VACCEL_OK);                                    \
                                                                               \
			ret = vaccel_arg_array_get_##TYPE_NAME##_array(        \
				&args, &get_buf, &get_count);                  \
			REQUIRE(ret == VACCEL_EINVAL);                         \
			REQUIRE(args.position == 1);                           \
                                                                               \
			void *tmp;                                             \
			size_t tmp_size;                                       \
			REQUIRE(vaccel_arg_array_get_buffer(                   \
					&args, &tmp, &tmp_size) == VACCEL_OK); \
		}                                                              \
                                                                               \
		SECTION("invalid buf")                                         \
		{                                                              \
			REQUIRE(vaccel_arg_array_add_##TYPE_NAME##_array(      \
					&args, buf, buf_count) == VACCEL_OK);  \
                                                                               \
			void *tmp = args.args[args.position].buf;              \
			args.args[args.position].buf = nullptr;                \
                                                                               \
			ret = vaccel_arg_array_get_##TYPE_NAME##_array(        \
				&args, &get_buf, &get_count);                  \
			REQUIRE(ret == VACCEL_EINVAL);                         \
			REQUIRE(args.position == 1);                           \
                                                                               \
			args.args[args.position].buf = tmp;                    \
			const size_t tmp_size = args.args[args.position].size; \
			args.args[args.position].size = 0;                     \
                                                                               \
			ret = vaccel_arg_array_get_##TYPE_NAME##_array(        \
				&args, &get_buf, &get_count);                  \
			REQUIRE(ret == VACCEL_EINVAL);                         \
			REQUIRE(args.position == 1);                           \
                                                                               \
			args.args[args.position].size = tmp_size;              \
			REQUIRE(vaccel_arg_array_get_##TYPE_NAME##_array(      \
					&args, &get_buf, &get_count) ==        \
				VACCEL_OK);                                    \
		}                                                              \
                                                                               \
		REQUIRE(vaccel_arg_array_release(&args) == VACCEL_OK);         \
	}

VACCEL_ARG_NUMERIC_TYPES(ARG_ARRAY_DEFINE_TEST_GET_FUNCS)

TEST_CASE("vaccel_arg_array_get_string", "[core][arg]")
{
	int ret;
	char buf[] = "test";
	char *get_buf;
	struct vaccel_arg_array args;

	REQUIRE(vaccel_arg_array_init(&args, 1) == VACCEL_OK);
	REQUIRE(vaccel_arg_array_add_string(&args, buf) == VACCEL_OK);

	SECTION("invalid arguments")
	{
		ret = vaccel_arg_array_get_string(nullptr, &get_buf);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 0);

		ret = vaccel_arg_array_get_string(&args, nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 0);
	}

	ret = vaccel_arg_array_get_string(&args, &get_buf);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(args.position == 1);
	REQUIRE(get_buf == buf);

	SECTION("out of range")
	{
		ret = vaccel_arg_array_get_string(&args, &get_buf);
		REQUIRE(ret == VACCEL_ERANGE);
		REQUIRE(args.position == 1);
	}

	SECTION("invalid type")
	{
		REQUIRE(vaccel_arg_array_add_char(&args, &buf[0]) == VACCEL_OK);

		ret = vaccel_arg_array_get_string(&args, &get_buf);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 1);

		char tmp;
		REQUIRE(vaccel_arg_array_get_char(&args, &tmp) == VACCEL_OK);
	}

	SECTION("invalid buf")
	{
		REQUIRE(vaccel_arg_array_add_string(&args, buf) == VACCEL_OK);

		void *tmp = args.args[args.position].buf;
		args.args[args.position].buf = nullptr;

		ret = vaccel_arg_array_get_string(&args, &get_buf);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 1);

		args.args[args.position].buf = tmp;
		const size_t tmp_size = args.args[args.position].size;
		args.args[args.position].size = 0;

		ret = vaccel_arg_array_get_string(&args, &get_buf);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 1);

		args.args[args.position].size = tmp_size;
		REQUIRE(vaccel_arg_array_get_string(&args, &get_buf) ==
			VACCEL_OK);
	}

	REQUIRE(vaccel_arg_array_release(&args) == VACCEL_OK);
}

TEST_CASE("vaccel_arg_array_get_buffer", "[core][arg]")
{
	int ret;
	char buf[] = "test";
	const size_t size = strlen(buf) + 1;
	void *get_buf;
	size_t get_size;
	struct vaccel_arg_array args;

	REQUIRE(vaccel_arg_array_init(&args, 1) == VACCEL_OK);
	REQUIRE(vaccel_arg_array_add_buffer(&args, buf, size) == VACCEL_OK);

	SECTION("invalid arguments")
	{
		ret = vaccel_arg_array_get_buffer(nullptr, &get_buf, &get_size);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 0);

		ret = vaccel_arg_array_get_buffer(&args, nullptr, &get_size);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 0);
	}

	ret = vaccel_arg_array_get_buffer(&args, &get_buf, &get_size);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(args.position == 1);
	REQUIRE(get_buf == buf);
	REQUIRE(get_size == size);

	SECTION("out of range")
	{
		ret = vaccel_arg_array_get_buffer(&args, &get_buf, &get_size);
		REQUIRE(ret == VACCEL_ERANGE);
		REQUIRE(args.position == 1);
	}

	SECTION("invalid type")
	{
		REQUIRE(vaccel_arg_array_add_char(&args, &buf[0]) == VACCEL_OK);

		ret = vaccel_arg_array_get_buffer(&args, &get_buf, &get_size);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 1);

		char tmp;
		REQUIRE(vaccel_arg_array_get_char(&args, &tmp) == VACCEL_OK);
	}

	SECTION("invalid buf")
	{
		REQUIRE(vaccel_arg_array_add_buffer(&args, buf, size) ==
			VACCEL_OK);

		void *tmp = args.args[args.position].buf;
		args.args[args.position].buf = nullptr;

		ret = vaccel_arg_array_get_buffer(&args, &get_buf, &get_size);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 1);

		args.args[args.position].buf = tmp;
		REQUIRE(vaccel_arg_array_get_buffer(&args, &get_buf,
						    &get_size) == VACCEL_OK);
	}

	REQUIRE(vaccel_arg_array_release(&args) == VACCEL_OK);
}

TEST_CASE("vaccel_arg_array_get_custom", "[core][arg]")
{
	int ret;
	vaccel_arg_type_t buf = VACCEL_ARG_RAW;
	const size_t size = sizeof(buf);
	vaccel_arg_type_t *get_buf;
	size_t get_size;
	struct vaccel_arg_array args;

	REQUIRE(vaccel_arg_array_init(&args, 1) == VACCEL_OK);
	REQUIRE(vaccel_arg_array_add_custom(&args, TEST_ARG_TYPE_ID,
					    (void *)&buf, size,
					    validate_arg_type) == VACCEL_OK);

	SECTION("invalid arguments")
	{
		ret = vaccel_arg_array_get_custom(nullptr, TEST_ARG_TYPE_ID,
						  (void **)&get_buf, &get_size,
						  validate_arg_type);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 0);

		ret = vaccel_arg_array_get_custom(&args, TEST_ARG_TYPE_ID,
						  (void **)&get_buf, &get_size,
						  nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 0);

		ret = vaccel_arg_array_get_custom(&args, TEST_ARG_TYPE_ID,
						  nullptr, &get_size,
						  validate_arg_type);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 0);
	}

	ret = vaccel_arg_array_get_custom(&args, TEST_ARG_TYPE_ID,
					  (void **)&get_buf, &get_size,
					  validate_arg_type);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(args.position == 1);
	REQUIRE(*get_buf == buf);
	REQUIRE(get_size == size);

	SECTION("out of range")
	{
		ret = vaccel_arg_array_get_custom(&args, TEST_ARG_TYPE_ID,
						  (void **)&get_buf, &get_size,
						  validate_arg_type);
		REQUIRE(ret == VACCEL_ERANGE);
		REQUIRE(args.position == 1);
	}

	SECTION("invalid type")
	{
		REQUIRE(vaccel_arg_array_add_raw(&args, (void *)&buf, size) ==
			VACCEL_OK);

		ret = vaccel_arg_array_get_custom(&args, TEST_ARG_TYPE_ID,
						  (void **)&get_buf, &get_size,
						  validate_arg_type);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 1);

		void *tmp;
		REQUIRE(vaccel_arg_array_get_raw(&args, &tmp, &get_size) ==
			VACCEL_OK);

		REQUIRE(vaccel_arg_array_add_custom(
				&args, TEST_ARG_TYPE_ID, (void *)&buf, size,
				validate_arg_type) == VACCEL_OK);

		ret = vaccel_arg_array_get_custom(&args, 0, (void **)&get_buf,
						  &get_size, validate_arg_type);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 2);

		REQUIRE(vaccel_arg_array_get_custom(
				&args, TEST_ARG_TYPE_ID, (void **)&get_buf,
				&get_size, validate_arg_type) == VACCEL_OK);
	}

	SECTION("invalid validator arguments")
	{
		REQUIRE(vaccel_arg_array_add_custom(
				&args, TEST_ARG_TYPE_ID, (void *)&buf, size,
				validate_arg_type) == VACCEL_OK);

		void *tmp = args.args[args.position].buf;
		args.args[args.position].buf = nullptr;

		ret = vaccel_arg_array_get_custom(&args, TEST_ARG_TYPE_ID,
						  (void **)&get_buf, &get_size,
						  validate_arg_type);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 1);

		args.args[args.position].buf = tmp;
		const size_t tmp_size = args.args[args.position].size;
		args.args[args.position].size = 0;

		ret = vaccel_arg_array_get_custom(&args, TEST_ARG_TYPE_ID,
						  (void **)&get_buf, &get_size,
						  validate_arg_type);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 1);

		args.args[args.position].size = tmp_size;
		REQUIRE(vaccel_arg_array_get_custom(
				&args, TEST_ARG_TYPE_ID, (void **)&get_buf,
				&get_size, validate_arg_type) == VACCEL_OK);
	}

	REQUIRE(vaccel_arg_array_release(&args) == VACCEL_OK);
}

TEST_CASE("vaccel_arg_array_get_serialized", "[core][arg]")
{
	int ret;
	struct mydata buf;
	const size_t size = sizeof(buf);
	struct mydata get_buf;
	const size_t mydata_count = 6;
	struct vaccel_arg_array args;

	buf.count = mydata_count;
	buf.array = (uint32_t *)malloc(buf.count * sizeof(*buf.array));
	REQUIRE(buf.array);

	for (uint32_t i = 0; i < buf.count; i++)
		buf.array[i] = 2 * i;

	REQUIRE(vaccel_arg_array_init(&args, 1) == VACCEL_OK);
	REQUIRE(vaccel_arg_array_add_serialized(&args, VACCEL_ARG_CUSTOM,
						MYDATA_TYPE_ID, &buf, size,
						mydata_serialize) == VACCEL_OK);

	SECTION("invalid arguments")
	{
		ret = vaccel_arg_array_get_serialized(nullptr,
						      VACCEL_ARG_CUSTOM,
						      MYDATA_TYPE_ID, &get_buf,
						      size, mydata_deserialize);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 0);

		ret = vaccel_arg_array_get_serialized(&args, VACCEL_ARG_CUSTOM,
						      MYDATA_TYPE_ID, nullptr,
						      size, mydata_deserialize);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 0);

		ret = vaccel_arg_array_get_serialized(&args, VACCEL_ARG_CUSTOM,
						      MYDATA_TYPE_ID, &get_buf,
						      size, nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 0);
	}

	ret = vaccel_arg_array_get_serialized(&args, VACCEL_ARG_CUSTOM,
					      MYDATA_TYPE_ID, &get_buf, size,
					      mydata_deserialize);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(args.position == 1);
	REQUIRE(get_buf.count == buf.count);
	for (uint32_t i = 0; i < get_buf.count; i++)
		REQUIRE(get_buf.array[i] == buf.array[i]);

	free(get_buf.array);

	SECTION("out of range")
	{
		ret = vaccel_arg_array_get_serialized(&args, VACCEL_ARG_CUSTOM,
						      MYDATA_TYPE_ID, &get_buf,
						      size, mydata_deserialize);
		REQUIRE(ret == VACCEL_ERANGE);
		REQUIRE(args.position == 1);
	}

	SECTION("invalid type")
	{
		REQUIRE(vaccel_arg_array_add_raw(&args, (void *)&buf, size) ==
			VACCEL_OK);

		ret = vaccel_arg_array_get_serialized(&args, VACCEL_ARG_CUSTOM,
						      MYDATA_TYPE_ID, &get_buf,
						      size, mydata_deserialize);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 1);

		void *tmp;
		size_t tmp_size;
		REQUIRE(vaccel_arg_array_get_raw(&args, &tmp, &tmp_size) ==
			VACCEL_OK);

		REQUIRE(vaccel_arg_array_add_serialized(
				&args, VACCEL_ARG_CUSTOM, MYDATA_TYPE_ID, &buf,
				size, mydata_serialize) == VACCEL_OK);

		ret = vaccel_arg_array_get_serialized(&args, VACCEL_ARG_CUSTOM,
						      0, &get_buf, size,
						      mydata_deserialize);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 2);

		REQUIRE(vaccel_arg_array_get_serialized(
				&args, VACCEL_ARG_CUSTOM, MYDATA_TYPE_ID,
				&get_buf, size,
				mydata_deserialize) == VACCEL_OK);

		free(get_buf.array);
	}

	SECTION("invalid deserializer arguments")
	{
		REQUIRE(vaccel_arg_array_add_serialized(
				&args, VACCEL_ARG_CUSTOM, MYDATA_TYPE_ID, &buf,
				size, mydata_serialize) == VACCEL_OK);

		void *tmp = args.args[args.position].buf;
		args.args[args.position].buf = nullptr;

		ret = vaccel_arg_array_get_serialized(&args, VACCEL_ARG_CUSTOM,
						      MYDATA_TYPE_ID, &get_buf,
						      size, mydata_deserialize);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 1);

		args.args[args.position].buf = tmp;

		ret = vaccel_arg_array_get_serialized(&args, VACCEL_ARG_CUSTOM,
						      MYDATA_TYPE_ID, &get_buf,
						      0, mydata_deserialize);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 1);

		REQUIRE(vaccel_arg_array_get_serialized(
				&args, VACCEL_ARG_CUSTOM, MYDATA_TYPE_ID,
				&get_buf, size,
				mydata_deserialize) == VACCEL_OK);

		free(get_buf.array);
	}

	REQUIRE(vaccel_arg_array_release(&args) == VACCEL_OK);
	free(buf.array);
}

TEST_CASE("vaccel_arg_array_set_raw", "[core][arg]")
{
	int ret;
	int32_t buf = 1;
	int32_t set_buf = 2;
	const size_t set_size = sizeof(set_buf);
	struct vaccel_arg_array args;

	REQUIRE(vaccel_arg_array_init(&args, 1) == VACCEL_OK);
	REQUIRE(vaccel_arg_array_add_raw(&args, &buf, sizeof(buf)) ==
		VACCEL_OK);

	SECTION("invalid arguments")
	{
		ret = vaccel_arg_array_set_raw(nullptr, &set_buf, set_size);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 0);

		ret = vaccel_arg_array_set_raw(&args, nullptr, set_size);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 0);

		ret = vaccel_arg_array_set_raw(&args, &set_buf,
					       sizeof(buf) + 1);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 0);
	}

	ret = vaccel_arg_array_set_raw(&args, &set_buf, set_size);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(args.position == 1);
	REQUIRE(*(int32_t *)args.args[args.position - 1].buf == set_buf);
	REQUIRE(args.args[args.position - 1].size == sizeof(buf));

	SECTION("out of range")
	{
		ret = vaccel_arg_array_set_raw(&args, &set_buf, set_size);
		REQUIRE(ret == VACCEL_ERANGE);
		REQUIRE(args.position == 1);
	}

	SECTION("invalid type")
	{
		REQUIRE(vaccel_arg_array_add_int32(&args, &buf) == VACCEL_OK);

		ret = vaccel_arg_array_set_raw(&args, &set_buf, set_size);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 1);
	}

	REQUIRE(vaccel_arg_array_release(&args) == VACCEL_OK);
}

#define ARG_ARRAY_DEFINE_TEST_SET_FUNCS(TYPE_NAME, C_TYPE, ARG_TYPE,           \
					ARRAY_TYPE)                            \
	TEST_CASE("vaccel_arg_array_set_" #TYPE_NAME, "[core][arg]")           \
	{                                                                      \
		int ret;                                                       \
		C_TYPE buf = 1;                                                \
		C_TYPE set_buf = 0;                                            \
		struct vaccel_arg_array args;                                  \
                                                                               \
		REQUIRE(vaccel_arg_array_init(&args, 1) == VACCEL_OK);         \
		REQUIRE(vaccel_arg_array_add_##TYPE_NAME(&args, &buf) ==       \
			VACCEL_OK);                                            \
                                                                               \
		SECTION("invalid arguments")                                   \
		{                                                              \
			ret = vaccel_arg_array_set_##TYPE_NAME(nullptr,        \
							       &set_buf);      \
			REQUIRE(ret == VACCEL_EINVAL);                         \
			REQUIRE(args.position == 0);                           \
                                                                               \
			ret = vaccel_arg_array_set_##TYPE_NAME(&args,          \
							       nullptr);       \
			REQUIRE(ret == VACCEL_EINVAL);                         \
			REQUIRE(args.position == 0);                           \
		}                                                              \
                                                                               \
		ret = vaccel_arg_array_set_##TYPE_NAME(&args, &set_buf);       \
		REQUIRE(ret == VACCEL_OK);                                     \
		REQUIRE(args.position == 1);                                   \
		REQUIRE(*(C_TYPE *)args.args[args.position - 1].buf ==         \
			set_buf);                                              \
                                                                               \
		SECTION("out of range")                                        \
		{                                                              \
			ret = vaccel_arg_array_set_##TYPE_NAME(&args,          \
							       &set_buf);      \
			REQUIRE(ret == VACCEL_ERANGE);                         \
			REQUIRE(args.position == 1);                           \
		}                                                              \
                                                                               \
		SECTION("invalid type")                                        \
		{                                                              \
			REQUIRE(vaccel_arg_array_add_buffer(&args, &buf,       \
							    sizeof(buf)) ==    \
				VACCEL_OK);                                    \
                                                                               \
			ret = vaccel_arg_array_set_##TYPE_NAME(&args,          \
							       &set_buf);      \
			REQUIRE(ret == VACCEL_EINVAL);                         \
			REQUIRE(args.position == 1);                           \
		}                                                              \
                                                                               \
		REQUIRE(vaccel_arg_array_release(&args) == VACCEL_OK);         \
	}                                                                      \
                                                                               \
	TEST_CASE("vaccel_arg_array_set_" #TYPE_NAME "_array", "[core][arg]")  \
	{                                                                      \
		int ret;                                                       \
		const size_t buf_count = 4;                                    \
		C_TYPE buf[buf_count] = { 1 };                                 \
		C_TYPE set_buf[buf_count] = { 0 };                             \
		struct vaccel_arg_array args;                                  \
                                                                               \
		REQUIRE(vaccel_arg_array_init(&args, 1) == VACCEL_OK);         \
		REQUIRE(vaccel_arg_array_add_##TYPE_NAME##_array(              \
				&args, buf, buf_count) == VACCEL_OK);          \
                                                                               \
		SECTION("invalid arguments")                                   \
		{                                                              \
			ret = vaccel_arg_array_set_##TYPE_NAME##_array(        \
				nullptr, set_buf, buf_count);                  \
			REQUIRE(ret == VACCEL_EINVAL);                         \
			REQUIRE(args.position == 0);                           \
                                                                               \
			ret = vaccel_arg_array_set_##TYPE_NAME##_array(        \
				&args, nullptr, buf_count);                    \
			REQUIRE(ret == VACCEL_EINVAL);                         \
			REQUIRE(args.position == 0);                           \
		}                                                              \
                                                                               \
		SECTION("too large")                                           \
		{                                                              \
			ret = vaccel_arg_array_set_##TYPE_NAME##_array(        \
				&args, set_buf, buf_count + 1);                \
			REQUIRE(ret == VACCEL_EINVAL);                         \
			REQUIRE(args.position == 0);                           \
		}                                                              \
                                                                               \
		ret = vaccel_arg_array_set_##TYPE_NAME##_array(&args, set_buf, \
							       buf_count);     \
		REQUIRE(ret == VACCEL_OK);                                     \
		REQUIRE(args.position == 1);                                   \
		REQUIRE(args.args[args.position - 1].size == sizeof(set_buf)); \
		for (size_t i = 0; i < buf_count; i++)                         \
			REQUIRE(((C_TYPE *)args.args[args.position - 1]        \
					 .buf)[i] == set_buf[i]);              \
                                                                               \
		SECTION("out of range")                                        \
		{                                                              \
			ret = vaccel_arg_array_set_##TYPE_NAME##_array(        \
				&args, set_buf, buf_count);                    \
			REQUIRE(ret == VACCEL_ERANGE);                         \
			REQUIRE(args.position == 1);                           \
		}                                                              \
                                                                               \
		SECTION("invalid type")                                        \
		{                                                              \
			REQUIRE(vaccel_arg_array_add_buffer(&args, buf,        \
							    sizeof(buf)) ==    \
				VACCEL_OK);                                    \
                                                                               \
			ret = vaccel_arg_array_set_##TYPE_NAME##_array(        \
				&args, set_buf, buf_count);                    \
			REQUIRE(ret == VACCEL_EINVAL);                         \
			REQUIRE(args.position == 1);                           \
		}                                                              \
                                                                               \
		REQUIRE(vaccel_arg_array_release(&args) == VACCEL_OK);         \
	}

VACCEL_ARG_NUMERIC_TYPES(ARG_ARRAY_DEFINE_TEST_SET_FUNCS)

TEST_CASE("vaccel_arg_array_set_string", "[core][arg]")
{
	int ret;
	char buf[] = "test1";
	const size_t size = strlen(buf) + 1;
	char set_buf[] = "test2";
	char set_buf_over[] = "test_2";
	struct vaccel_arg_array args;

	REQUIRE(vaccel_arg_array_init(&args, 1) == VACCEL_OK);
	REQUIRE(vaccel_arg_array_add_string(&args, buf) == VACCEL_OK);

	SECTION("invalid arguments")
	{
		ret = vaccel_arg_array_set_string(nullptr, set_buf);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 0);

		ret = vaccel_arg_array_set_string(&args, nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 0);

		void *tmp = args.args[args.position].buf;
		args.args[args.position].buf = nullptr;

		ret = vaccel_arg_array_set_string(&args, set_buf);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 0);

		args.args[args.position].buf = tmp;
		ret = vaccel_arg_array_set_string(&args, set_buf_over);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 0);
	}

	ret = vaccel_arg_array_set_string(&args, set_buf);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(args.position == 1);
	REQUIRE(args.args[args.position - 1].buf == buf);
	REQUIRE(args.args[args.position - 1].size == size);
	REQUIRE(strcmp((const char *)args.args[args.position - 1].buf,
		       set_buf) == 0);

	SECTION("out of range")
	{
		ret = vaccel_arg_array_set_string(&args, set_buf);
		REQUIRE(ret == VACCEL_ERANGE);
		REQUIRE(args.position == 1);
	}

	SECTION("invalid type")
	{
		REQUIRE(vaccel_arg_array_add_char(&args, &buf[0]) == VACCEL_OK);

		ret = vaccel_arg_array_set_string(&args, set_buf);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 1);
	}

	REQUIRE(vaccel_arg_array_release(&args) == VACCEL_OK);
}

TEST_CASE("vaccel_arg_array_set_buffer", "[core][arg]")
{
	int ret;
	char buf[] = "test1";
	const size_t size = strlen(buf) + 1;
	char set_buf[] = "test";
	const size_t set_size = strlen(set_buf) + 1;
	char set_buf_over[] = "test_2";
	const size_t set_over_size = strlen(set_buf_over) + 1;
	struct vaccel_arg_array args;

	REQUIRE(vaccel_arg_array_init(&args, 1) == VACCEL_OK);
	REQUIRE(vaccel_arg_array_add_buffer(&args, buf, size) == VACCEL_OK);

	SECTION("invalid arguments")
	{
		ret = vaccel_arg_array_set_buffer(nullptr, set_buf, set_size);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 0);

		ret = vaccel_arg_array_set_buffer(&args, nullptr, set_size);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 0);

		void *tmp = args.args[args.position].buf;
		args.args[args.position].buf = nullptr;

		ret = vaccel_arg_array_set_buffer(&args, set_buf, set_size);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 0);

		args.args[args.position].buf = tmp;
		ret = vaccel_arg_array_set_buffer(&args, set_buf_over,
						  set_over_size);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 0);
	}

	ret = vaccel_arg_array_set_buffer(&args, set_buf, set_size);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(args.position == 1);
	REQUIRE(args.args[args.position - 1].buf == buf);
	REQUIRE(args.args[args.position - 1].size == size);
	REQUIRE(strcmp((const char *)args.args[args.position - 1].buf,
		       set_buf) == 0);

	SECTION("out of range")
	{
		ret = vaccel_arg_array_set_buffer(&args, set_buf, set_size);
		REQUIRE(ret == VACCEL_ERANGE);
		REQUIRE(args.position == 1);
	}

	SECTION("invalid type")
	{
		REQUIRE(vaccel_arg_array_add_char(&args, &buf[0]) == VACCEL_OK);

		ret = vaccel_arg_array_set_buffer(&args, set_buf, set_size);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 1);

		char tmp;
		REQUIRE(vaccel_arg_array_get_char(&args, &tmp) == VACCEL_OK);
	}

	REQUIRE(vaccel_arg_array_release(&args) == VACCEL_OK);
}

TEST_CASE("vaccel_arg_array_set_custom", "[core][arg]")
{
	int ret;
	vaccel_arg_type_t buf = VACCEL_ARG_RAW;
	const size_t size = sizeof(buf);
	vaccel_arg_type_t set_buf = VACCEL_ARG_BOOL;
	struct vaccel_arg_array args;

	REQUIRE(vaccel_arg_array_init(&args, 1) == VACCEL_OK);
	REQUIRE(vaccel_arg_array_add_custom(&args, TEST_ARG_TYPE_ID,
					    (void *)&buf, size,
					    validate_arg_type) == VACCEL_OK);

	SECTION("invalid arguments")
	{
		ret = vaccel_arg_array_set_custom(nullptr, TEST_ARG_TYPE_ID,
						  (void *)&set_buf, size,
						  validate_arg_type);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 0);

		ret = vaccel_arg_array_set_custom(&args, TEST_ARG_TYPE_ID,
						  (void *)&set_buf, size,
						  nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 0);

		ret = vaccel_arg_array_set_custom(&args, TEST_ARG_TYPE_ID,
						  (void *)&set_buf, size - 1,
						  validate_arg_type);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 0);
	}

	SECTION("invalid custom type")
	{
		ret = vaccel_arg_array_set_custom(&args, 0, (void *)&set_buf,
						  size, validate_arg_type);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 0);
	}

	SECTION("invalid validator arguments")
	{
		ret = vaccel_arg_array_set_custom(&args, TEST_ARG_TYPE_ID,
						  nullptr, size,
						  validate_arg_type);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 0);
	}

	ret = vaccel_arg_array_set_custom(&args, TEST_ARG_TYPE_ID,
					  (void *)&set_buf, size,
					  validate_arg_type);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(args.position == 1);
	REQUIRE(args.args[args.position - 1].size == size);
	REQUIRE(*(vaccel_arg_type_t *)args.args[args.position - 1].buf ==
		set_buf);

	SECTION("out of range")
	{
		ret = vaccel_arg_array_set_custom(&args, TEST_ARG_TYPE_ID,
						  (void *)&set_buf, size,
						  validate_arg_type);
		REQUIRE(ret == VACCEL_ERANGE);
		REQUIRE(args.position == 1);
	}

	SECTION("invalid type")
	{
		REQUIRE(vaccel_arg_array_add_raw(&args, (void *)&buf, size) ==
			VACCEL_OK);

		ret = vaccel_arg_array_set_custom(&args, TEST_ARG_TYPE_ID,
						  (void *)&set_buf, size,
						  validate_arg_type);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 1);
	}

	REQUIRE(vaccel_arg_array_release(&args) == VACCEL_OK);
}

TEST_CASE("vaccel_arg_array_set_serialized", "[core][arg]")
{
	int ret;
	struct mydata buf;
	const size_t size = sizeof(buf);
	struct mydata set_buf;
	struct mydata deser_buf;
	const size_t mydata_count = 6;
	struct vaccel_arg_array args;

	buf.count = mydata_count;
	buf.array = (uint32_t *)malloc(buf.count * sizeof(*buf.array));
	REQUIRE(buf.array);

	set_buf.count = mydata_count;
	set_buf.array =
		(uint32_t *)malloc(set_buf.count * sizeof(*set_buf.array));
	REQUIRE(set_buf.array);

	for (uint32_t i = 0; i < buf.count; i++)
		buf.array[i] = 2 * i;

	for (uint32_t i = 0; i < set_buf.count; i++)
		set_buf.array[i] = i;

	REQUIRE(vaccel_arg_array_init(&args, 1) == VACCEL_OK);
	REQUIRE(vaccel_arg_array_add_serialized(&args, VACCEL_ARG_CUSTOM,
						MYDATA_TYPE_ID, &buf, size,
						mydata_serialize) == VACCEL_OK);

	SECTION("invalid arguments")
	{
		ret = vaccel_arg_array_set_serialized(nullptr,
						      VACCEL_ARG_CUSTOM,
						      MYDATA_TYPE_ID, &set_buf,
						      size, mydata_serialize);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 0);

		ret = vaccel_arg_array_set_serialized(&args, VACCEL_ARG_CUSTOM,
						      MYDATA_TYPE_ID, nullptr,
						      size, mydata_serialize);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 0);

		ret = vaccel_arg_array_set_serialized(&args, VACCEL_ARG_CUSTOM,
						      MYDATA_TYPE_ID, &set_buf,
						      size, nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 0);
	}

	ret = vaccel_arg_array_set_serialized(&args, VACCEL_ARG_CUSTOM,
					      MYDATA_TYPE_ID, &set_buf, size,
					      mydata_serialize);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(args.position == 1);

	// verify data
	vaccel_arg_array_reset_position(&args);
	REQUIRE(vaccel_arg_array_get_serialized(
			&args, VACCEL_ARG_CUSTOM, MYDATA_TYPE_ID, &deser_buf,
			size, mydata_deserialize) == VACCEL_OK);
	REQUIRE(deser_buf.count == set_buf.count);
	for (uint32_t i = 0; i < set_buf.count; i++)
		REQUIRE(deser_buf.array[i] == set_buf.array[i]);

	SECTION("out of range")
	{
		ret = vaccel_arg_array_set_serialized(&args, VACCEL_ARG_CUSTOM,
						      MYDATA_TYPE_ID, &set_buf,
						      size, mydata_serialize);
		REQUIRE(ret == VACCEL_ERANGE);
		REQUIRE(args.position == 1);
	}

	SECTION("invalid type")
	{
		REQUIRE(vaccel_arg_array_add_raw(&args, (void *)&buf, size) ==
			VACCEL_OK);

		ret = vaccel_arg_array_set_serialized(&args, VACCEL_ARG_CUSTOM,
						      MYDATA_TYPE_ID, &set_buf,
						      size, mydata_serialize);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 1);

		void *tmp;
		size_t tmp_size;
		REQUIRE(vaccel_arg_array_get_raw(&args, &tmp, &tmp_size) ==
			VACCEL_OK);

		REQUIRE(vaccel_arg_array_add_serialized(
				&args, VACCEL_ARG_CUSTOM, MYDATA_TYPE_ID, &buf,
				size, mydata_serialize) == VACCEL_OK);

		ret = vaccel_arg_array_set_serialized(&args, VACCEL_ARG_CUSTOM,
						      0, &set_buf, size,
						      mydata_serialize);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 2);

		REQUIRE(vaccel_arg_array_set_serialized(
				&args, VACCEL_ARG_CUSTOM, MYDATA_TYPE_ID,
				&set_buf, size, mydata_serialize) == VACCEL_OK);
	}

	SECTION("invalid serializer arguments")
	{
		REQUIRE(vaccel_arg_array_add_serialized(
				&args, VACCEL_ARG_CUSTOM, MYDATA_TYPE_ID, &buf,
				size, mydata_serialize) == VACCEL_OK);

		ret = vaccel_arg_array_set_serialized(&args, VACCEL_ARG_CUSTOM,
						      MYDATA_TYPE_ID, &set_buf,
						      0, mydata_serialize);
		REQUIRE(ret == VACCEL_EINVAL);
		REQUIRE(args.position == 1);

		REQUIRE(vaccel_arg_array_set_serialized(
				&args, VACCEL_ARG_CUSTOM, MYDATA_TYPE_ID,
				&set_buf, size, mydata_serialize) == VACCEL_OK);
	}

	REQUIRE(vaccel_arg_array_release(&args) == VACCEL_OK);
	free(buf.array);
	free(set_buf.array);
	free(deser_buf.array);
}

TEST_CASE("vaccel_arg_array_get_remaining", "[core][arg]")
{
	int ret;
	const size_t buf_count = 16;
	int32_t buf[buf_count];
	struct vaccel_arg_array args;
	struct vaccel_arg *get_raw_args;
	size_t get_count;

	REQUIRE(vaccel_arg_array_init(&args, buf_count) == VACCEL_OK);
	for (size_t i = 0; i < buf_count; i++) {
		buf[i] = (int32_t)i;
		REQUIRE(vaccel_arg_array_add_int32(&args, &buf[i]) ==
			VACCEL_OK);
	}

	for (size_t i = 0; i < buf_count / 2; i++) {
		int32_t tmp;
		REQUIRE(vaccel_arg_array_get_int32(&args, &tmp) == VACCEL_OK);
	}

	ret = vaccel_arg_array_get_remaining(&args, &get_raw_args, &get_count);
	REQUIRE(ret == VACCEL_OK);
	REQUIRE(get_count == buf_count / 2);
	for (size_t i = 0; i < buf_count / 2; i++)
		REQUIRE(*(int32_t *)get_raw_args[i].buf ==
			buf[buf_count / 2 + i]);

	SECTION("invalid arguments")
	{
		ret = vaccel_arg_array_get_remaining(nullptr, &get_raw_args,
						     &get_count);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_arg_array_get_remaining(&args, nullptr,
						     &get_count);
		REQUIRE(ret == VACCEL_EINVAL);

		ret = vaccel_arg_array_get_remaining(&args, &get_raw_args,
						     nullptr);
		REQUIRE(ret == VACCEL_EINVAL);
	}

	SECTION("no remaining")
	{
		for (size_t i = 0; i < buf_count / 2; i++) {
			int32_t tmp;
			REQUIRE(vaccel_arg_array_get_int32(&args, &tmp) ==
				VACCEL_OK);
		}

		ret = vaccel_arg_array_get_remaining(&args, &get_raw_args,
						     &get_count);
		REQUIRE(ret == VACCEL_OK);
		REQUIRE(get_count == 0);
		REQUIRE(get_raw_args == nullptr);
	}

	REQUIRE(vaccel_arg_array_release(&args) == VACCEL_OK);
}

TEST_CASE("vaccel_arg_array_remaining_count", "[core][arg]")
{
	const size_t buf_count = 16;
	int32_t buf[buf_count];
	struct vaccel_arg_array args;
	size_t get_count;

	REQUIRE(vaccel_arg_array_init(&args, buf_count) == VACCEL_OK);
	for (size_t i = 0; i < buf_count; i++) {
		buf[i] = (int32_t)i;
		REQUIRE(vaccel_arg_array_add_int32(&args, &buf[i]) ==
			VACCEL_OK);
	}

	for (size_t i = 0; i < buf_count / 2; i++) {
		int32_t tmp;
		REQUIRE(vaccel_arg_array_get_int32(&args, &tmp) == VACCEL_OK);
	}

	get_count = vaccel_arg_array_remaining_count(&args);
	REQUIRE(get_count == buf_count / 2);

	SECTION("invalid arguments")
	{
		get_count = vaccel_arg_array_remaining_count(nullptr);
		REQUIRE(get_count == 0);
	}

	SECTION("no remaining")
	{
		for (size_t i = 0; i < buf_count / 2; i++) {
			int32_t tmp;
			REQUIRE(vaccel_arg_array_get_int32(&args, &tmp) ==
				VACCEL_OK);
		}

		get_count = vaccel_arg_array_remaining_count(nullptr);
		REQUIRE(get_count == 0);
	}

	REQUIRE(vaccel_arg_array_release(&args) == VACCEL_OK);
}

TEST_CASE("vaccel_arg_array_position", "[core][arg]")
{
	const size_t buf_count = 4;
	int32_t buf[buf_count];
	struct vaccel_arg_array args;
	size_t get_position;

	REQUIRE(vaccel_arg_array_init(&args, buf_count) == VACCEL_OK);
	for (size_t i = 0; i < buf_count; i++) {
		buf[i] = (int32_t)i;
		REQUIRE(vaccel_arg_array_add_int32(&args, &buf[i]) ==
			VACCEL_OK);
	}

	for (size_t i = 0; i < buf_count / 2; i++) {
		int32_t tmp;
		REQUIRE(vaccel_arg_array_get_int32(&args, &tmp) == VACCEL_OK);
	}

	get_position = vaccel_arg_array_position(&args);
	REQUIRE(get_position == buf_count / 2);

	SECTION("invalid arguments")
	{
		get_position = vaccel_arg_array_remaining_count(nullptr);
		REQUIRE(get_position == 0);
	}

	REQUIRE(vaccel_arg_array_release(&args) == VACCEL_OK);
}

TEST_CASE("vaccel_arg_array_set_position", "[core][arg]")
{
	const size_t buf_count = 4;
	int32_t buf[buf_count];
	struct vaccel_arg_array args;

	REQUIRE(vaccel_arg_array_init(&args, buf_count) == VACCEL_OK);
	for (size_t i = 0; i < buf_count; i++) {
		buf[i] = (int32_t)i;
		REQUIRE(vaccel_arg_array_add_int32(&args, &buf[i]) ==
			VACCEL_OK);
	}

	for (size_t i = 0; i < buf_count / 2; i++) {
		int32_t tmp;
		REQUIRE(vaccel_arg_array_get_int32(&args, &tmp) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		vaccel_arg_array_set_position(nullptr, 0);
		REQUIRE(args.position == buf_count / 2);
	}

	SECTION("out of range")
	{
		vaccel_arg_array_set_position(&args, args.count + 1);
		REQUIRE(args.position == args.count);
	}

	vaccel_arg_array_set_position(&args, 0);
	REQUIRE(args.position == 0);
	REQUIRE(*(int32_t *)args.args[args.position].buf ==
		(int32_t)args.position);

	REQUIRE(vaccel_arg_array_release(&args) == VACCEL_OK);
}

TEST_CASE("vaccel_arg_array_reset_position", "[core][arg]")
{
	const size_t buf_count = 4;
	int32_t buf[buf_count];
	struct vaccel_arg_array args;

	REQUIRE(vaccel_arg_array_init(&args, buf_count) == VACCEL_OK);
	for (size_t i = 0; i < buf_count; i++) {
		buf[i] = (int32_t)i;
		REQUIRE(vaccel_arg_array_add_int32(&args, &buf[i]) ==
			VACCEL_OK);
	}

	for (size_t i = 0; i < buf_count / 2; i++) {
		int32_t tmp;
		REQUIRE(vaccel_arg_array_get_int32(&args, &tmp) == VACCEL_OK);
	}

	SECTION("invalid arguments")
	{
		vaccel_arg_array_reset_position(nullptr);
		REQUIRE(args.position == buf_count / 2);
	}

	vaccel_arg_array_reset_position(&args);
	REQUIRE(args.position == 0);
	REQUIRE(*(int32_t *)args.args[args.position].buf ==
		(int32_t)args.position);

	REQUIRE(vaccel_arg_array_release(&args) == VACCEL_OK);
}

TEST_CASE("vaccel_arg_array_count", "[core][arg]")
{
	const size_t buf_count = 4;
	int32_t buf = 1;
	struct vaccel_arg_array args;
	size_t get_count;

	REQUIRE(vaccel_arg_array_init(&args, buf_count) == VACCEL_OK);
	for (size_t i = 0; i < buf_count; i++)
		REQUIRE(vaccel_arg_array_add_int32(&args, &buf) == VACCEL_OK);

	SECTION("invalid arguments")
	{
		get_count = vaccel_arg_array_count(nullptr);
		REQUIRE(get_count == 0);
	}

	get_count = vaccel_arg_array_count(&args);
	REQUIRE(get_count == args.count);

	REQUIRE(vaccel_arg_array_release(&args) == VACCEL_OK);
}

TEST_CASE("vaccel_arg_array_raw", "[core][arg]")
{
	const size_t buf_count = 4;
	int32_t buf = 1;
	struct vaccel_arg_array args;
	struct vaccel_arg *get_raw_args;

	REQUIRE(vaccel_arg_array_init(&args, buf_count) == VACCEL_OK);
	for (size_t i = 0; i < buf_count; i++)
		REQUIRE(vaccel_arg_array_add_int32(&args, &buf) == VACCEL_OK);

	SECTION("invalid arguments")
	{
		get_raw_args = vaccel_arg_array_raw(nullptr);
		REQUIRE(get_raw_args == nullptr);
	}

	get_raw_args = vaccel_arg_array_raw(&args);
	REQUIRE(get_raw_args == args.args);

	REQUIRE(vaccel_arg_array_release(&args) == VACCEL_OK);
}
