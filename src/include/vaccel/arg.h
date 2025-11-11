// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "utils/enum.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Define vaccel_arg_type_t, vaccel_arg_type_to_str() and
 * vaccel_arg_type_to_base_str() */
#define _ENUM_PREFIX VACCEL_ARG
#define VACCEL_ARG_TYPE_ENUM_LIST(VACCEL_ENUM_ITEM)   \
	VACCEL_ENUM_ITEM(RAW, 0, _ENUM_PREFIX)        \
	VACCEL_ENUM_ITEM(INT8, _ENUM_PREFIX)          \
	VACCEL_ENUM_ITEM(INT8_ARRAY, _ENUM_PREFIX)    \
	VACCEL_ENUM_ITEM(INT16, _ENUM_PREFIX)         \
	VACCEL_ENUM_ITEM(INT16_ARRAY, _ENUM_PREFIX)   \
	VACCEL_ENUM_ITEM(INT32, _ENUM_PREFIX)         \
	VACCEL_ENUM_ITEM(INT32_ARRAY, _ENUM_PREFIX)   \
	VACCEL_ENUM_ITEM(INT64, _ENUM_PREFIX)         \
	VACCEL_ENUM_ITEM(INT64_ARRAY, _ENUM_PREFIX)   \
	VACCEL_ENUM_ITEM(UINT8, _ENUM_PREFIX)         \
	VACCEL_ENUM_ITEM(UINT8_ARRAY, _ENUM_PREFIX)   \
	VACCEL_ENUM_ITEM(UINT16, _ENUM_PREFIX)        \
	VACCEL_ENUM_ITEM(UINT16_ARRAY, _ENUM_PREFIX)  \
	VACCEL_ENUM_ITEM(UINT32, _ENUM_PREFIX)        \
	VACCEL_ENUM_ITEM(UINT32_ARRAY, _ENUM_PREFIX)  \
	VACCEL_ENUM_ITEM(UINT64, _ENUM_PREFIX)        \
	VACCEL_ENUM_ITEM(UINT64_ARRAY, _ENUM_PREFIX)  \
	VACCEL_ENUM_ITEM(FLOAT32, _ENUM_PREFIX)       \
	VACCEL_ENUM_ITEM(FLOAT32_ARRAY, _ENUM_PREFIX) \
	VACCEL_ENUM_ITEM(FLOAT64, _ENUM_PREFIX)       \
	VACCEL_ENUM_ITEM(FLOAT64_ARRAY, _ENUM_PREFIX) \
	VACCEL_ENUM_ITEM(BOOL, _ENUM_PREFIX)          \
	VACCEL_ENUM_ITEM(BOOL_ARRAY, _ENUM_PREFIX)    \
	VACCEL_ENUM_ITEM(CHAR, _ENUM_PREFIX)          \
	VACCEL_ENUM_ITEM(CHAR_ARRAY, _ENUM_PREFIX)    \
	VACCEL_ENUM_ITEM(UCHAR, _ENUM_PREFIX)         \
	VACCEL_ENUM_ITEM(UCHAR_ARRAY, _ENUM_PREFIX)   \
	VACCEL_ENUM_ITEM(STRING, _ENUM_PREFIX)        \
	VACCEL_ENUM_ITEM(BUFFER, _ENUM_PREFIX)        \
	VACCEL_ENUM_ITEM(CUSTOM, _ENUM_PREFIX)

VACCEL_ENUM_DEF_WITH_STR_FUNCS(vaccel_arg_type, _ENUM_PREFIX,
			       VACCEL_ARG_TYPE_ENUM_LIST)
#undef _ENUM_PREFIX

/* X-macro to define validation functions */
#define VACCEL_ARG_NUMERIC_TYPES(X)                                     \
	X(int8, int8_t, VACCEL_ARG_INT8, VACCEL_ARG_INT8_ARRAY)         \
	X(int16, int16_t, VACCEL_ARG_INT16, VACCEL_ARG_INT16_ARRAY)     \
	X(int32, int32_t, VACCEL_ARG_INT32, VACCEL_ARG_INT32_ARRAY)     \
	X(int64, int64_t, VACCEL_ARG_INT64, VACCEL_ARG_INT64_ARRAY)     \
	X(uint8, uint8_t, VACCEL_ARG_UINT8, VACCEL_ARG_UINT8_ARRAY)     \
	X(uint16, uint16_t, VACCEL_ARG_UINT16, VACCEL_ARG_UINT16_ARRAY) \
	X(uint32, uint32_t, VACCEL_ARG_UINT32, VACCEL_ARG_UINT32_ARRAY) \
	X(uint64, uint64_t, VACCEL_ARG_UINT64, VACCEL_ARG_UINT64_ARRAY) \
	X(float, float, VACCEL_ARG_FLOAT32, VACCEL_ARG_FLOAT32_ARRAY)   \
	X(double, double, VACCEL_ARG_FLOAT64, VACCEL_ARG_FLOAT64_ARRAY) \
	X(bool, bool, VACCEL_ARG_BOOL, VACCEL_ARG_BOOL_ARRAY)           \
	X(char, char, VACCEL_ARG_CHAR, VACCEL_ARG_CHAR_ARRAY)           \
	X(uchar, unsigned char, VACCEL_ARG_UCHAR, VACCEL_ARG_UCHAR_ARRAY)

/* Generic argument container */
struct vaccel_arg {
	/* arg data */
	void *buf;

	/* size of the arg data */
	size_t size;

	/* type of the arg data */
	vaccel_arg_type_t type;

	/* ID of custom type, if type is VACCEL_ARG_CUSTOM */
	uint32_t custom_type_id;

	/* true if arg data is allocated by the API */
	bool owned;
};

/* Initialize arg with owned data (buffer will be copied) */
int vaccel_arg_init(struct vaccel_arg *arg, const void *buf, size_t size,
		    vaccel_arg_type_t type, uint32_t custom_id);

/* Initialize arg from provided buffer (caller retains ownership) */
int vaccel_arg_init_from_buf(struct vaccel_arg *arg, void *buf, size_t size,
			     vaccel_arg_type_t type, uint32_t custom_id);

/* Release arg data */
int vaccel_arg_release(struct vaccel_arg *arg);

/* Allocate and initialize arg with owned data (buffer will be copied) */
int vaccel_arg_new(struct vaccel_arg **arg, const void *buf, size_t size,
		   vaccel_arg_type_t type, uint32_t custom_id);

/* Allocate and initialize arg from provided buffer (caller retains
 * ownership) */
int vaccel_arg_from_buf(struct vaccel_arg **arg, void *buf, size_t size,
			vaccel_arg_type_t type, uint32_t custom_id);

/* Release arg data and free arg created with `vaccel_arg_new()` */
int vaccel_arg_delete(struct vaccel_arg *arg);

/* Argument array container */
struct vaccel_arg_array {
	/* arg array */
	struct vaccel_arg *args;

	/* count of array args; used when adding args */
	size_t count;

	/* total capacity of the array */
	size_t capacity;

	/* sequential array position; used when getting args */
	size_t position;

	/* true if arg array is allocated by the API */
	bool owned;
};

/* Custom validator function type */
typedef bool (*vaccel_arg_type_validator_fn)(const void *buf, size_t size,
					     uint32_t custom_id);

/* Serializer/Deserializer function types */
typedef int (*vaccel_arg_serializer_fn)(const void *data, size_t data_size,
					uint32_t custom_id, void **buf,
					size_t *size);
typedef int (*vaccel_arg_deserializer_fn)(const void *buf, size_t size,
					  uint32_t custom_id, size_t data_size,
					  void *data);

/* Initialize arg array structure */
int vaccel_arg_array_init(struct vaccel_arg_array *array,
			  size_t initial_capacity);

/* Release arg array data */
int vaccel_arg_array_release(struct vaccel_arg_array *array);

/* Allocate and initialize arg array structure */
int vaccel_arg_array_new(struct vaccel_arg_array **array,
			 size_t initial_capacity);

/* Release arg array data and free arg array created with
 * `vaccel_arg_array_new()` */
int vaccel_arg_array_delete(struct vaccel_arg_array *array);

/* Wrap existing array of args with the container */
int vaccel_arg_array_wrap(struct vaccel_arg_array *array,
			  struct vaccel_arg *args, size_t count);

/* Clear contained arg data and reset position/count.
 * NOTE: Unlike `vaccel_arg_array_release()`, this will not free the contained
 * arg array. */
void vaccel_arg_array_clear(struct vaccel_arg_array *array);

/* Add raw buffer (no validation) */
int vaccel_arg_array_add_raw(struct vaccel_arg_array *array, void *buf,
			     size_t size);

/* Declare type-specific add functions (auto-generated) */
#define VACCEL_ARG_ARRAY_DECLARE_ADD_FUNCS(TYPE_NAME, C_TYPE, ARG_TYPE,      \
					   ARRAY_TYPE)                       \
	int vaccel_arg_array_add_##TYPE_NAME(struct vaccel_arg_array *array, \
					     C_TYPE *value);                 \
	int vaccel_arg_array_add_##TYPE_NAME##_array(                        \
		struct vaccel_arg_array *array, C_TYPE *values, size_t count);

VACCEL_ARG_NUMERIC_TYPES(VACCEL_ARG_ARRAY_DECLARE_ADD_FUNCS)

/* Add string */
int vaccel_arg_array_add_string(struct vaccel_arg_array *array, char *str);

/* Add buffer */
int vaccel_arg_array_add_buffer(struct vaccel_arg_array *array, void *buf,
				size_t size);

/* Add custom type with validator */
int vaccel_arg_array_add_custom(struct vaccel_arg_array *array,
				uint32_t custom_id, void *buf, size_t size,
				vaccel_arg_type_validator_fn validator);

/* Add serialized data */
int vaccel_arg_array_add_serialized(struct vaccel_arg_array *array,
				    vaccel_arg_type_t type, uint32_t custom_id,
				    const void *data, size_t data_size,
				    vaccel_arg_serializer_fn serializer);

/* Add range of args from source array (ignores source position) */
int vaccel_arg_array_add_range(struct vaccel_arg_array *dest,
			       const struct vaccel_arg_array *src,
			       size_t start_idx, size_t count, bool copy);

/* Add remaining args from source array's current position */
int vaccel_arg_array_add_remaining(struct vaccel_arg_array *dest,
				   const struct vaccel_arg_array *src,
				   bool copy);

/* Add all args from source array (ignores source position) */
int vaccel_arg_array_add_all(struct vaccel_arg_array *dest,
			     const struct vaccel_arg_array *src, bool copy);

/* Get next raw buffer (no validation) */
int vaccel_arg_array_get_raw(struct vaccel_arg_array *array, void **buf,
			     size_t *size);

/* Declare type-specific get functions (auto-generated) */
#define VACCEL_ARG_ARRAY_DECLARE_GET_FUNCS(TYPE_NAME, C_TYPE, ARG_TYPE,      \
					   ARRAY_TYPE)                       \
	int vaccel_arg_array_get_##TYPE_NAME(struct vaccel_arg_array *array, \
					     C_TYPE *value);                 \
	int vaccel_arg_array_get_##TYPE_NAME##_array(                        \
		struct vaccel_arg_array *array, C_TYPE **values,             \
		size_t *count);

VACCEL_ARG_NUMERIC_TYPES(VACCEL_ARG_ARRAY_DECLARE_GET_FUNCS)

/* Get string */
int vaccel_arg_array_get_string(struct vaccel_arg_array *array, char **str);

/* Get buffer */
int vaccel_arg_array_get_buffer(struct vaccel_arg_array *array, void **buf,
				size_t *size);

/* Get next custom type (validates with provided validator) */
int vaccel_arg_array_get_custom(struct vaccel_arg_array *array,
				uint32_t expected_id, void **buf, size_t *size,
				vaccel_arg_type_validator_fn validator);

/* Get deserialized data */
int vaccel_arg_array_get_serialized(struct vaccel_arg_array *array,
				    vaccel_arg_type_t expected_type,
				    uint32_t expected_custom_id, void *data,
				    size_t expected_size,
				    vaccel_arg_deserializer_fn deserializer);

/* Set next raw buffer (no validation) */
int vaccel_arg_array_set_raw(struct vaccel_arg_array *array, void *buf,
			     size_t size);

/* Declare type-specific set functions (auto-generated) */
#define VACCEL_ARG_ARRAY_DECLARE_SET_FUNCS(TYPE_NAME, C_TYPE, ARG_TYPE,      \
					   ARRAY_TYPE)                       \
	int vaccel_arg_array_set_##TYPE_NAME(struct vaccel_arg_array *array, \
					     C_TYPE *value);                 \
	int vaccel_arg_array_set_##TYPE_NAME##_array(                        \
		struct vaccel_arg_array *array, C_TYPE *values, size_t count);

VACCEL_ARG_NUMERIC_TYPES(VACCEL_ARG_ARRAY_DECLARE_SET_FUNCS)

/* Set string */
int vaccel_arg_array_set_string(struct vaccel_arg_array *array, char *str);

/* Set buffer */
int vaccel_arg_array_set_buffer(struct vaccel_arg_array *array, void *buf,
				size_t size);

/* Set custom type (validates with provided validator) */
int vaccel_arg_array_set_custom(struct vaccel_arg_array *array,
				uint32_t expected_id, void *buf, size_t size,
				vaccel_arg_type_validator_fn validator);

/* Set serialized data */
int vaccel_arg_array_set_serialized(struct vaccel_arg_array *array,
				    vaccel_arg_type_t expected_type,
				    uint32_t expected_custom_id, void *data,
				    size_t data_size,
				    vaccel_arg_serializer_fn serializer);

/* Get remaining args from current position */
int vaccel_arg_array_get_remaining(struct vaccel_arg_array *array,
				   struct vaccel_arg **args, size_t *count);

/* Get count of remaining args */
size_t vaccel_arg_array_remaining_count(const struct vaccel_arg_array *array);

/* Get current position */
size_t vaccel_arg_array_position(const struct vaccel_arg_array *array);

/* Set current position (for getting args) */
void vaccel_arg_array_set_position(struct vaccel_arg_array *array,
				   size_t position);

/* Reset position (for getting args) */
void vaccel_arg_array_reset_position(struct vaccel_arg_array *array);

/* Get count of total args */
size_t vaccel_arg_array_count(const struct vaccel_arg_array *array);

/* Get raw array of args */
struct vaccel_arg *vaccel_arg_array_raw(struct vaccel_arg_array *array);

/*
 * Deprecated. To be removed.
 */

struct vaccel_arg_list {
	/* total size of the list */
	uint32_t size;

	/* pointer to the entries of the list */
	struct vaccel_arg *list;

	/* current index to add the next entry */
	int curr_idx;

	/*
	 * array that holds 1 in positions that memory is allocated using
	 * malloc(), so it can be freed later automatically
	 */
	int *idcs_allocated_space;
};

/* Initializes the arg-list structure */
__attribute__((deprecated(
	"The function will be removed in a future release"))) struct vaccel_arg_list *
vaccel_args_init(uint32_t size);

/* Add a serialized argument in the list */
__attribute__((
	deprecated("The function will be removed in a future release"))) int
vaccel_add_serial_arg(struct vaccel_arg_list *args, void *buf, uint32_t size);

/* Add a non-serialized argument in the list */
__attribute__((
	deprecated("The function will be removed in a future release"))) int
vaccel_add_nonserial_arg(struct vaccel_arg_list *args, void *buf, uint32_t type,
			 void *(*serializer)(void *, uint32_t *));

/* Define an expected argument (serialized) */
__attribute__((
	deprecated("The function will be removed in a future release"))) int
vaccel_expect_serial_arg(struct vaccel_arg_list *args, void *buf,
			 uint32_t size);

/* Define an expected non-serialized argument */
__attribute__((
	deprecated("The function will be removed in a future release"))) int
vaccel_expect_nonserial_arg(struct vaccel_arg_list *args,
			    uint32_t expected_size);

/* Extract a serialized argument out of the list */
__attribute__((
	deprecated("The function will be removed in a future release"))) void *
vaccel_extract_serial_arg(struct vaccel_arg *args, int idx);

/* Extract a non-serialized argument out of the list */
__attribute__((
	deprecated("The function will be removed in a future release"))) void *
vaccel_extract_nonserial_arg(struct vaccel_arg *args, int idx,
			     void *(*deserializer)(void *, uint32_t));

/* Write to an expected serialized argument. Used inside plugin. */
__attribute__((
	deprecated("The function will be removed in a future release"))) int
vaccel_write_serial_arg(struct vaccel_arg *args, int idx, void *buf);

/* Write to an expected non-serialized argument. Used inside plugin. */
__attribute__((
	deprecated("The function will be removed in a future release"))) int
vaccel_write_nonserial_arg(struct vaccel_arg *args, int idx, void *buf,
			   void *(*serializer)(void *, uint32_t *));

/* Delete any allocated memory in the arg-list structure */
__attribute__((
	deprecated("The function will be removed in a future release"))) int
vaccel_delete_args(struct vaccel_arg_list *args);

#ifdef __cplusplus
}
#endif
