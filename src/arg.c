// SPDX-License-Identifier: Apache-2.0

#include "arg.h"
#include "error.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct type_info {
	uint16_t element_size;
	uint8_t flags;
};

/* Generic types */
#define TYPE_VARIABLE_SIZE 0x01
#define TYPE_ARRAY 0x02
#define TYPE_BOOL_VALUES 0x04

/* Macro to generate type descriptors */
#define ARG_TYPE_DESCRIPTOR(TYPE_NAME, C_TYPE, ARG_TYPE, ARRAY_TYPE)           \
	[ARG_TYPE] = { sizeof(C_TYPE),                                         \
		       (ARG_TYPE == VACCEL_ARG_BOOL) ? TYPE_BOOL_VALUES : 0 }, \
	[ARRAY_TYPE] = { sizeof(C_TYPE),                                       \
			 TYPE_ARRAY | ((ARRAY_TYPE == VACCEL_ARG_BOOL_ARRAY) ? \
					       TYPE_BOOL_VALUES :              \
					       0) },

static const struct type_info type_descriptors[] = {
	[VACCEL_ARG_RAW] = { 0, TYPE_VARIABLE_SIZE },
	/* Generate type descriptors using X-macro */
	VACCEL_ARG_NUMERIC_TYPES(
		ARG_TYPE_DESCRIPTOR)[VACCEL_ARG_STRING] = { 0,
							    TYPE_VARIABLE_SIZE },
	[VACCEL_ARG_BUFFER] = { 0, TYPE_VARIABLE_SIZE },
	[VACCEL_ARG_CUSTOM] = { 0, TYPE_VARIABLE_SIZE }
};

#define TYPE_COUNT (sizeof(type_descriptors) / sizeof(type_descriptors[0]))

/* Define type-specific add functions (auto-generated) */
#define ARG_ARRAY_DEFINE_ADD_FUNCS(TYPE_NAME, C_TYPE, ARG_TYPE, ARRAY_TYPE)   \
	int vaccel_arg_array_add_##TYPE_NAME(struct vaccel_arg_array *array,  \
					     C_TYPE *value)                   \
	{                                                                     \
		if (!array)                                                   \
			return VACCEL_EINVAL;                                 \
                                                                              \
		return vaccel_arg_array_add_validated(                        \
			array, value, sizeof(C_TYPE), ARG_TYPE, 0, false);    \
	}                                                                     \
                                                                              \
	int vaccel_arg_array_add_##TYPE_NAME##_array(                         \
		struct vaccel_arg_array *array, C_TYPE *values, size_t count) \
	{                                                                     \
		if (!array || !values || !count)                              \
			return VACCEL_EINVAL;                                 \
                                                                              \
		size_t total_size = count * sizeof(C_TYPE);                   \
		return vaccel_arg_array_add_validated(                        \
			array, values, total_size, ARRAY_TYPE, 0, false);     \
	}

/* Define type-specific get functions (auto-generated) */
#define ARG_ARRAY_DEFINE_GET_FUNCS(TYPE_NAME, C_TYPE, ARG_TYPE, ARRAY_TYPE)   \
	static int arg_array_get_##TYPE_NAME##_arg(                           \
		struct vaccel_arg_array *array, struct vaccel_arg **arg)      \
	{                                                                     \
		if (!array || !arg)                                           \
			return VACCEL_EINVAL;                                 \
		if (array->position >= array->count)                          \
			return VACCEL_ERANGE;                                 \
                                                                              \
		*arg = &array->args[array->position];                         \
		if (!*arg)                                                    \
			return VACCEL_EINVAL;                                 \
                                                                              \
		if ((*arg)->type != ARG_TYPE)                                 \
			return VACCEL_EINVAL;                                 \
		if ((*arg)->size != sizeof(C_TYPE))                           \
			return VACCEL_EINVAL;                                 \
		if (!(*arg)->buf)                                             \
			return VACCEL_EINVAL;                                 \
                                                                              \
		return VACCEL_OK;                                             \
	}                                                                     \
	int vaccel_arg_array_get_##TYPE_NAME(struct vaccel_arg_array *array,  \
					     C_TYPE *value)                   \
	{                                                                     \
		if (!array || !value)                                         \
			return VACCEL_EINVAL;                                 \
                                                                              \
		struct vaccel_arg *arg;                                       \
		int ret = arg_array_get_##TYPE_NAME##_arg(array, &arg);       \
		if (ret)                                                      \
			return ret;                                           \
                                                                              \
		*value = *(C_TYPE *)arg->buf;                                 \
		array->position++;                                            \
		return VACCEL_OK;                                             \
	}                                                                     \
                                                                              \
	static int arg_array_get_##TYPE_NAME##_array_arg(                     \
		struct vaccel_arg_array *array, struct vaccel_arg **arg)      \
	{                                                                     \
		if (!array || !arg)                                           \
			return VACCEL_EINVAL;                                 \
		if (array->position >= array->count)                          \
			return VACCEL_ERANGE;                                 \
                                                                              \
		*arg = &array->args[array->position];                         \
		if (!*arg)                                                    \
			return VACCEL_EINVAL;                                 \
                                                                              \
		if ((*arg)->type != ARRAY_TYPE)                               \
			return VACCEL_EINVAL;                                 \
		if (!(*arg)->buf || !(*arg)->size)                            \
			return VACCEL_EINVAL;                                 \
		if ((*arg)->size % sizeof(C_TYPE) != 0)                       \
			return VACCEL_EINVAL;                                 \
                                                                              \
		return VACCEL_OK;                                             \
	}                                                                     \
                                                                              \
	int vaccel_arg_array_get_##TYPE_NAME##_array(                         \
		struct vaccel_arg_array *array, C_TYPE **values,              \
		size_t *count)                                                \
	{                                                                     \
		if (!array || !values)                                        \
			return VACCEL_EINVAL;                                 \
                                                                              \
		struct vaccel_arg *arg;                                       \
		int ret = arg_array_get_##TYPE_NAME##_array_arg(array, &arg); \
		if (ret)                                                      \
			return ret;                                           \
                                                                              \
		*values = (C_TYPE *)arg->buf;                                 \
		if (count)                                                    \
			*count = arg->size / sizeof(C_TYPE);                  \
                                                                              \
		array->position++;                                            \
		return VACCEL_OK;                                             \
	}

/* Define type-specific set functions (auto-generated) */
#define ARG_ARRAY_DEFINE_SET_FUNCS(TYPE_NAME, C_TYPE, ARG_TYPE, ARRAY_TYPE)   \
	int vaccel_arg_array_set_##TYPE_NAME(struct vaccel_arg_array *array,  \
					     C_TYPE *value)                   \
	{                                                                     \
		if (!array || !value)                                         \
			return VACCEL_EINVAL;                                 \
                                                                              \
		struct vaccel_arg *arg;                                       \
		int ret = arg_array_get_##TYPE_NAME##_arg(array, &arg);       \
		if (ret)                                                      \
			return ret;                                           \
                                                                              \
		*(C_TYPE *)arg->buf = *value;                                 \
		array->position++;                                            \
		return VACCEL_OK;                                             \
	}                                                                     \
                                                                              \
	int vaccel_arg_array_set_##TYPE_NAME##_array(                         \
		struct vaccel_arg_array *array, C_TYPE *values, size_t count) \
	{                                                                     \
		if (!array || !values)                                        \
			return VACCEL_EINVAL;                                 \
                                                                              \
		struct vaccel_arg *arg;                                       \
		int ret = arg_array_get_##TYPE_NAME##_array_arg(array, &arg); \
		if (ret)                                                      \
			return ret;                                           \
                                                                              \
		size_t size = count * sizeof(C_TYPE);                         \
		if (arg->size < size)                                         \
			return VACCEL_EINVAL;                                 \
                                                                              \
		memcpy(arg->buf, values, size);                               \
		array->position++;                                            \
		return VACCEL_OK;                                             \
	}

int vaccel_arg_init(struct vaccel_arg *arg, const void *buf, size_t size,
		    vaccel_arg_type_t type, uint32_t custom_id)
{
	if (!arg || (buf && !size))
		return VACCEL_EINVAL;

	void *owned_buf = NULL;
	if (size > 0) {
		owned_buf = malloc(size);
		if (!owned_buf)
			return VACCEL_ENOMEM;

		if (buf)
			memcpy(owned_buf, buf, size);
		else
			memset(owned_buf, 0, size);
	}

	arg->buf = owned_buf;
	arg->size = size;
	arg->type = type;
	arg->custom_type_id = custom_id;
	arg->owned = true;

	return VACCEL_OK;
}

int vaccel_arg_init_from_buf(struct vaccel_arg *arg, void *buf, size_t size,
			     vaccel_arg_type_t type, uint32_t custom_id)
{
	if (!arg || (buf && !size) || (size && !buf))
		return VACCEL_EINVAL;

	arg->buf = buf;
	arg->size = size;
	arg->type = type;
	arg->custom_type_id = custom_id;
	arg->owned = false;

	return VACCEL_OK;
}

int vaccel_arg_release(struct vaccel_arg *arg)
{
	if (!arg)
		return VACCEL_EINVAL;

	if (arg->owned)
		free(arg->buf);

	arg->buf = NULL;
	arg->size = 0;
	arg->type = VACCEL_ARG_MAX;
	arg->custom_type_id = 0;
	arg->owned = false;

	return VACCEL_OK;
}

int vaccel_arg_new(struct vaccel_arg **arg, const void *buf, size_t size,
		   vaccel_arg_type_t type, uint32_t custom_id)
{
	if (!arg)
		return VACCEL_EINVAL;

	struct vaccel_arg *a =
		(struct vaccel_arg *)malloc(sizeof(struct vaccel_arg));
	if (!a)
		return VACCEL_ENOMEM;

	int ret = vaccel_arg_init(a, buf, size, type, custom_id);
	if (ret) {
		free(a);
		return ret;
	}

	*arg = a;
	return VACCEL_OK;
}

int vaccel_arg_from_buf(struct vaccel_arg **arg, void *buf, size_t size,
			vaccel_arg_type_t type, uint32_t custom_id)
{
	if (!arg)
		return VACCEL_EINVAL;

	struct vaccel_arg *a =
		(struct vaccel_arg *)malloc(sizeof(struct vaccel_arg));
	if (!a)
		return VACCEL_ENOMEM;

	int ret = vaccel_arg_init_from_buf(a, buf, size, type, custom_id);
	if (ret) {
		free(a);
		return ret;
	}

	*arg = a;
	return VACCEL_OK;
}

int vaccel_arg_delete(struct vaccel_arg *arg)
{
	int ret = vaccel_arg_release(arg);
	if (ret)
		return ret;

	free(arg);
	return VACCEL_OK;
}

int vaccel_arg_array_init(struct vaccel_arg_array *array,
			  size_t initial_capacity)
{
	if (!array)
		return VACCEL_EINVAL;

	array->count = 0;
	array->position = 0;
	array->owned = true;
	array->capacity = initial_capacity > 0 ? initial_capacity :
						 ARG_ARRAY_CAPACITY_DEFAULT;
	array->args = calloc(array->capacity, sizeof(struct vaccel_arg));

	return array->args ? VACCEL_OK : VACCEL_ENOMEM;
}

int vaccel_arg_array_release(struct vaccel_arg_array *array)
{
	if (!array || !array->args)
		return VACCEL_EINVAL;

	if (array->owned) {
		for (size_t i = 0; i < array->count; i++)
			vaccel_arg_release(&array->args[i]);

		free(array->args);
	}

	array->args = NULL;
	array->count = 0;
	array->capacity = 0;
	array->position = 0;
	array->owned = false;

	return VACCEL_OK;
}

int vaccel_arg_array_new(struct vaccel_arg_array **array,
			 size_t initial_capacity)
{
	if (!array)
		return VACCEL_EINVAL;

	struct vaccel_arg_array *a = (struct vaccel_arg_array *)malloc(
		sizeof(struct vaccel_arg_array));
	if (!a)
		return VACCEL_ENOMEM;

	int ret = vaccel_arg_array_init(a, initial_capacity);
	if (ret) {
		free(a);
		return ret;
	}

	*array = a;
	return VACCEL_OK;
}

int vaccel_arg_array_delete(struct vaccel_arg_array *array)
{
	int ret = vaccel_arg_array_release(array);
	if (ret)
		return ret;

	free(array);
	return VACCEL_OK;
}

int vaccel_arg_array_wrap(struct vaccel_arg_array *array,
			  struct vaccel_arg *args, size_t count)
{
	if (!array || (!args && count > 0))
		return VACCEL_EINVAL;

	array->args = args;
	array->count = count;
	array->capacity = count; /* Wrapped arrays are fixed size */
	array->position = 0;
	array->owned = false;

	return VACCEL_OK;
}

void vaccel_arg_array_clear(struct vaccel_arg_array *array)
{
	if (!array || !array->args)
		return;

	for (size_t i = 0; i < array->count; i++)
		vaccel_arg_release(&array->args[i]);

	/* Reset counters but keep capacity */
	array->count = 0;
	array->position = 0;
}

static int arg_array_grow(struct vaccel_arg_array *array)
{
	if (!array->owned)
		return VACCEL_ERANGE;

	size_t new_capacity = array->capacity * 2;
	struct vaccel_arg *new_args =
		realloc(array->args, new_capacity * sizeof(struct vaccel_arg));
	if (!new_args)
		return VACCEL_ENOMEM;

	array->args = new_args;
	array->capacity = new_capacity;
	return VACCEL_OK;
}

static bool validate_bool_value(const void *buf, size_t size)
{
	if (buf == NULL || size != sizeof(bool))
		return false;

	bool val = *(const bool *)buf;
	return val == 0 || val == 1;
}

static bool validate_bool_array(const void *buf, size_t size)
{
	if (buf == NULL || size == 0 || (size % sizeof(bool)) != 0)
		return false;

	const bool *vals = (const bool *)buf;
	size_t count = size / sizeof(bool);
	for (size_t i = 0; i < count; i++) {
		if (vals[i] != 0 && vals[i] != 1)
			return false;
	}
	return true;
}

static bool validate_builtin_type(vaccel_arg_type_t type, const void *buf,
				  size_t size)
{
	if (type >= TYPE_COUNT)
		return false;

	const struct type_info *info = &type_descriptors[type];

	/* Variable size types */
	if (info->flags & TYPE_VARIABLE_SIZE) {
		if (type == VACCEL_ARG_RAW || type == VACCEL_ARG_CUSTOM) {
			return true;
		}
		if (type == VACCEL_ARG_STRING) {
			return buf != NULL && size > 0 &&
			       ((char *)buf)[size - 1] == '\0';
		}
		return buf != NULL && size > 0; /* BUFFER */
	}

	/* Fixed size validation */
	if (info->flags & TYPE_ARRAY) {
		if (buf == NULL || size == 0 ||
		    (size % info->element_size) != 0)
			return false;

		/* Bool array needs special validation */
		if (info->flags & TYPE_BOOL_VALUES)
			return validate_bool_array(buf, size);
	} else {
		if (buf == NULL || size != info->element_size)
			return false;

		/* Bool value needs special validation */
		if (info->flags & TYPE_BOOL_VALUES)
			return validate_bool_value(buf, size);
	}

	return true;
}

static int vaccel_arg_array_add_validated(struct vaccel_arg_array *array,
					  void *buf, size_t size,
					  vaccel_arg_type_t type,
					  uint32_t custom_id, bool owned)
{
	if (!array)
		return VACCEL_EINVAL;

	if (array->count >= array->capacity) {
		int ret = arg_array_grow(array);
		if (ret)
			return ret;
	}

	/* Validate builtin types */
	if (type != VACCEL_ARG_RAW && type != VACCEL_ARG_CUSTOM) {
		if (!validate_builtin_type(type, buf, size))
			return VACCEL_EINVAL;
	}

	struct vaccel_arg *arg = &array->args[array->count];
	arg->buf = buf;
	arg->size = size;
	arg->type = type;
	arg->custom_type_id = custom_id;
	arg->owned = owned;

	array->count++;
	return VACCEL_OK;
}

int vaccel_arg_array_add_raw(struct vaccel_arg_array *array, void *buf,
			     size_t size)
{
	if (!array)
		return VACCEL_EINVAL;

	if (array->count >= array->capacity) {
		int ret = arg_array_grow(array);
		if (ret)
			return ret;
	}

	int ret = vaccel_arg_init_from_buf(&array->args[array->count], buf,
					   size, VACCEL_ARG_RAW, 0);
	if (ret)
		return ret;

	array->count++;
	return VACCEL_OK;
}

VACCEL_ARG_NUMERIC_TYPES(ARG_ARRAY_DEFINE_ADD_FUNCS)

int vaccel_arg_array_add_string(struct vaccel_arg_array *array, char *str)
{
	if (!array || !str)
		return VACCEL_EINVAL;

	size_t size = strlen(str) + 1;
	return vaccel_arg_array_add_validated(array, str, size,
					      VACCEL_ARG_STRING, 0, false);
}

int vaccel_arg_array_add_buffer(struct vaccel_arg_array *array, void *buf,
				size_t size)
{
	return vaccel_arg_array_add_validated(array, buf, size,
					      VACCEL_ARG_BUFFER, 0, false);
}

int vaccel_arg_array_add_custom(struct vaccel_arg_array *array,
				uint32_t custom_id, void *buf, size_t size,
				vaccel_arg_type_validator_fn validator)
{
	if (!array || !validator)
		return VACCEL_EINVAL;

	if (!validator(buf, size, custom_id))
		return VACCEL_EINVAL;

	return vaccel_arg_array_add_validated(
		array, buf, size, VACCEL_ARG_CUSTOM, custom_id, false);
}

int vaccel_arg_array_add_serialized(struct vaccel_arg_array *array,
				    vaccel_arg_type_t type, uint32_t custom_id,
				    const void *data, size_t data_size,
				    vaccel_arg_serializer_fn serializer)
{
	if (!array || !serializer)
		return VACCEL_EINVAL;

	void *buf;
	size_t size;
	int ret = serializer(data, data_size, custom_id, &buf, &size);
	if (ret)
		return ret;

	ret = vaccel_arg_array_add_validated(array, buf, size, type, custom_id,
					     true);
	if (ret) {
		free(buf);
		return ret;
	}

	return VACCEL_OK;
}

static int vaccel_arg_copy_buf(const struct vaccel_arg *src,
			       struct vaccel_arg *dest, bool copy)
{
	if (!src || !dest)
		return VACCEL_EINVAL;

	dest->size = src->size;
	dest->type = src->type;
	dest->custom_type_id = src->custom_type_id;

	if (!copy || !src->buf) {
		dest->buf = src->buf;
		dest->owned = false;
	} else {
		dest->buf = malloc(src->size);
		if (!dest->buf)
			return VACCEL_ENOMEM;

		memcpy(dest->buf, src->buf, src->size);
		dest->owned = true;
	}

	return VACCEL_OK;
}

int vaccel_arg_array_add_range(struct vaccel_arg_array *dest,
			       const struct vaccel_arg_array *src,
			       size_t start_idx, size_t count, bool copy)
{
	if (!dest || !src)
		return VACCEL_EINVAL;
	if (start_idx >= src->count || start_idx + count > src->count)
		return VACCEL_ERANGE;
	if (count == 0)
		return VACCEL_OK;

	while (dest->count + count > dest->capacity) {
		int ret = arg_array_grow(dest);
		if (ret != VACCEL_OK)
			return ret;
	}

	for (size_t i = 0; i < count; i++) {
		const struct vaccel_arg *src_arg = &src->args[start_idx + i];
		struct vaccel_arg *dest_arg = &dest->args[dest->count + i];

		int ret = vaccel_arg_copy_buf(src_arg, dest_arg, copy);
		if (ret) {
			for (size_t j = 0; j < i; j++) {
				if (copy && dest->args[dest->count + j].buf)
					free(dest->args[dest->count + j].buf);
			}
			return ret;
		}
	}

	dest->count += count;
	return VACCEL_OK;
}

int vaccel_arg_array_add_remaining(struct vaccel_arg_array *dest,
				   const struct vaccel_arg_array *src,
				   bool copy)
{
	if (!dest || !src)
		return VACCEL_EINVAL;

	/* Nothing to add */
	if (src->position >= src->count)
		return VACCEL_OK;

	size_t rem_count = vaccel_arg_array_remaining_count(src);
	return vaccel_arg_array_add_range(dest, src, src->position, rem_count,
					  copy);
}

int vaccel_arg_array_add_all(struct vaccel_arg_array *dest,
			     const struct vaccel_arg_array *src, bool copy)
{
	if (!dest || !src)
		return VACCEL_EINVAL;

	/* Nothing to add */
	if (src->count == 0)
		return VACCEL_OK;

	return vaccel_arg_array_add_range(dest, src, 0, src->count, copy);
}

int vaccel_arg_array_get_raw(struct vaccel_arg_array *array, void **buf,
			     size_t *size)
{
	if (!array || !buf || !size)
		return VACCEL_EINVAL;
	if (array->position >= array->count)
		return VACCEL_ERANGE;

	struct vaccel_arg *arg = &array->args[array->position];
	if (!arg)
		return VACCEL_EINVAL;

	if (arg->type != VACCEL_ARG_RAW)
		return VACCEL_EINVAL;

	*buf = arg->buf;
	*size = arg->size;

	array->position++;
	return VACCEL_OK;
}

VACCEL_ARG_NUMERIC_TYPES(ARG_ARRAY_DEFINE_GET_FUNCS)

int vaccel_arg_array_get_string(struct vaccel_arg_array *array, char **str)
{
	if (!array || !str)
		return VACCEL_EINVAL;
	if (array->position >= array->count)
		return VACCEL_ERANGE;

	struct vaccel_arg *arg = &array->args[array->position];
	if (!arg)
		return VACCEL_EINVAL;

	if (arg->type != VACCEL_ARG_STRING)
		return VACCEL_EINVAL;
	if (!arg->buf || arg->size == 0)
		return VACCEL_EINVAL;

	char *s = (char *)arg->buf;
	if (s[arg->size - 1] != '\0')
		return VACCEL_EINVAL;

	*str = s;
	array->position++;
	return VACCEL_OK;
}

int vaccel_arg_array_get_buffer(struct vaccel_arg_array *array, void **buf,
				size_t *size)
{
	if (!array || !buf || !size)
		return VACCEL_EINVAL;
	if (array->position >= array->count)
		return VACCEL_ERANGE;

	struct vaccel_arg *arg = &array->args[array->position];
	if (!arg)
		return VACCEL_EINVAL;

	if (arg->type != VACCEL_ARG_BUFFER)
		return VACCEL_EINVAL;
	if (!arg->buf || arg->size == 0)
		return VACCEL_EINVAL;

	*buf = arg->buf;
	*size = arg->size;

	array->position++;
	return VACCEL_OK;
}

int vaccel_arg_array_get_custom(struct vaccel_arg_array *array,
				uint32_t expected_id, void **buf, size_t *size,
				vaccel_arg_type_validator_fn validator)
{
	if (!array || !validator || !buf)
		return VACCEL_EINVAL;
	if (array->position >= array->count)
		return VACCEL_ERANGE;

	struct vaccel_arg *arg = &array->args[array->position];
	if (!arg)
		return VACCEL_EINVAL;

	if (arg->type != VACCEL_ARG_CUSTOM)
		return VACCEL_EINVAL;
	if (arg->custom_type_id != expected_id)
		return VACCEL_EINVAL;

	if (!validator(arg->buf, arg->size, arg->custom_type_id))
		return VACCEL_EINVAL;

	*buf = arg->buf;
	if (size)
		*size = arg->size;

	array->position++;
	return VACCEL_OK;
}

int vaccel_arg_array_get_serialized(struct vaccel_arg_array *array,
				    vaccel_arg_type_t expected_type,
				    uint32_t expected_custom_id, void *data,
				    size_t expected_size,
				    vaccel_arg_deserializer_fn deserializer)
{
	if (!array || !data || !deserializer)
		return VACCEL_EINVAL;
	if (array->position >= array->count)
		return VACCEL_ERANGE;

	struct vaccel_arg *arg = &array->args[array->position];
	if (!arg)
		return VACCEL_EINVAL;

	if (arg->type != expected_type)
		return VACCEL_EINVAL;
	if (arg->type == VACCEL_ARG_CUSTOM) {
		if (arg->custom_type_id != expected_custom_id)
			return VACCEL_EINVAL;
	} else {
		if (!validate_builtin_type(arg->type, arg->buf, arg->size))
			return VACCEL_EINVAL;
		expected_custom_id = 0;
	}

	int ret = deserializer(arg->buf, arg->size, expected_custom_id,
			       expected_size, data);
	if (ret)
		return ret;

	array->position++;
	return VACCEL_OK;
}

int vaccel_arg_array_set_raw(struct vaccel_arg_array *array, void *buf,
			     size_t size)
{
	if (!array)
		return VACCEL_EINVAL;
	if (array->position >= array->count)
		return VACCEL_ERANGE;

	struct vaccel_arg *arg = &array->args[array->position];
	if (!arg)
		return VACCEL_EINVAL;

	if (arg->type != VACCEL_ARG_RAW)
		return VACCEL_EINVAL;

	if (buf && size) {
		if (!arg->buf || arg->size < size)
			return VACCEL_EINVAL;

		memcpy(arg->buf, buf, size);
	} else if (arg->buf && arg->size) {
		return VACCEL_EINVAL;
	}

	array->position++;
	return VACCEL_OK;
}

VACCEL_ARG_NUMERIC_TYPES(ARG_ARRAY_DEFINE_SET_FUNCS)

int vaccel_arg_array_set_string(struct vaccel_arg_array *array, char *str)
{
	if (!array || !str)
		return VACCEL_EINVAL;
	if (array->position >= array->count)
		return VACCEL_ERANGE;

	struct vaccel_arg *arg = &array->args[array->position];
	if (!arg)
		return VACCEL_EINVAL;

	if (arg->type != VACCEL_ARG_STRING)
		return VACCEL_EINVAL;
	if (!arg->buf || arg->size == 0)
		return VACCEL_EINVAL;

	size_t size = strlen(str) + 1;
	if (arg->size < size)
		return VACCEL_EINVAL;

	strncpy((char *)arg->buf, str, arg->size);
	array->position++;
	return VACCEL_OK;
}

int vaccel_arg_array_set_buffer(struct vaccel_arg_array *array, void *buf,
				size_t size)
{
	if (!array || !buf || !size)
		return VACCEL_EINVAL;
	if (array->position >= array->count)
		return VACCEL_ERANGE;

	struct vaccel_arg *arg = &array->args[array->position];
	if (!arg)
		return VACCEL_EINVAL;

	if (arg->type != VACCEL_ARG_BUFFER)
		return VACCEL_EINVAL;
	if (!arg->buf || arg->size < size)
		return VACCEL_EINVAL;

	memcpy(arg->buf, buf, size);
	array->position++;
	return VACCEL_OK;
}

int vaccel_arg_array_set_custom(struct vaccel_arg_array *array,
				uint32_t expected_id, void *buf, size_t size,
				vaccel_arg_type_validator_fn validator)
{
	if (!array || !validator)
		return VACCEL_EINVAL;
	if (array->position >= array->count)
		return VACCEL_ERANGE;

	struct vaccel_arg *arg = &array->args[array->position];
	if (!arg)
		return VACCEL_EINVAL;

	if (arg->type != VACCEL_ARG_CUSTOM)
		return VACCEL_EINVAL;
	if (arg->custom_type_id != expected_id)
		return VACCEL_EINVAL;
	if (arg->size != size)
		return VACCEL_EINVAL;

	if (!validator(buf, size, arg->custom_type_id))
		return VACCEL_EINVAL;

	memcpy(arg->buf, buf, size);
	array->position++;
	return VACCEL_OK;
}

int vaccel_arg_array_set_serialized(struct vaccel_arg_array *array,
				    vaccel_arg_type_t expected_type,
				    uint32_t expected_custom_id, void *data,
				    size_t data_size,
				    vaccel_arg_serializer_fn serializer)
{
	if (!array || !data || !serializer)
		return VACCEL_EINVAL;
	if (array->position >= array->count)
		return VACCEL_ERANGE;

	struct vaccel_arg *arg = &array->args[array->position];
	if (!arg)
		return VACCEL_EINVAL;

	if (arg->type != expected_type)
		return VACCEL_EINVAL;
	if (arg->type == VACCEL_ARG_CUSTOM) {
		if (arg->custom_type_id != expected_custom_id)
			return VACCEL_EINVAL;
	} else {
		if (!validate_builtin_type(arg->type, arg->buf, arg->size))
			return VACCEL_EINVAL;
		expected_custom_id = 0;
	}

	void *buf;
	size_t size;
	int ret = serializer(data, data_size, expected_custom_id, &buf, &size);
	if (ret)
		return ret;
	if (arg->size != size) {
		free(buf);
		return VACCEL_EINVAL;
	}

	memcpy(arg->buf, buf, size);
	free(buf);

	array->position++;
	return VACCEL_OK;
}

int vaccel_arg_array_get_remaining(struct vaccel_arg_array *array,
				   struct vaccel_arg **args, size_t *count)
{
	if (!array || !args || !count)
		return VACCEL_EINVAL;

	if (array->position >= array->count) {
		*args = NULL;
		*count = 0;
		return VACCEL_OK;
	}

	*args = &array->args[array->position];
	*count = array->count - array->position;

	return VACCEL_OK;
}

size_t vaccel_arg_array_remaining_count(const struct vaccel_arg_array *array)
{
	if (!array || array->position >= array->count)
		return 0;
	return array->count - array->position;
}

size_t vaccel_arg_array_position(const struct vaccel_arg_array *array)
{
	return array ? array->position : 0;
}

void vaccel_arg_array_set_position(struct vaccel_arg_array *array,
				   size_t position)
{
	if (array) {
		array->position = position > array->count ? array->count :
							    position;
	}
}

void vaccel_arg_array_reset_position(struct vaccel_arg_array *array)
{
	if (array)
		array->position = 0;
}

size_t vaccel_arg_array_count(const struct vaccel_arg_array *array)
{
	return array ? array->count : 0;
}

struct vaccel_arg *vaccel_arg_array_raw(struct vaccel_arg_array *array)
{
	return array ? array->args : NULL;
}
