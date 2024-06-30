// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "error.h"
#include <stddef.h>
#include <stdint.h>

struct vaccel_arg {
	uint32_t argtype;

	uint32_t size;

	void *buf;
};

struct vaccel_arg_list {
	/* total size of the list */
	uint32_t size;

	/* pointer to the entries of the list*/
	struct vaccel_arg *list;

	/* current index to add the next entry */
	int curr_idx;

	/*
    An array that holds 1 in positions
    that memory is allocated using malloc(), 
    so it can be freed later automatically.  
    */
	int *idcs_allocated_space;
};

/* Initializes the arg-list structure */
struct vaccel_arg_list *vaccel_args_init(uint32_t size);

/* Add a serialized argument in the list */
int vaccel_add_serial_arg(struct vaccel_arg_list *args, void *buf,
			  uint32_t size);

/* Add a non-serialized argument in the list */
int vaccel_add_nonserial_arg(struct vaccel_arg_list *args, void *buf,
			     uint32_t argtype,
			     void *(*serializer)(void *, uint32_t *));

/* Define an expected argument (serialized) */
int vaccel_expect_serial_arg(struct vaccel_arg_list *args, void *buf,
			     uint32_t size);

/* Define an expected non-serialized argument */
int vaccel_expect_nonserial_arg(struct vaccel_arg_list *args,
				uint32_t expected_size);

/* Extract a serialized argument out of the list */
void *vaccel_extract_serial_arg(struct vaccel_arg *args, int idx);

/* Extract a non-serialized argument out of the list */
void *vaccel_extract_nonserial_arg(struct vaccel_arg *args, int idx,
				   void *(*deserializer)(void *, uint32_t));

/* 
Write to an expected serialized argument.
Used inside plugin. 
*/
int vaccel_write_serial_arg(struct vaccel_arg *args, int idx, void *buf);

/* 
Write to an expected non-serialized argument.
Used inside plugin. 
*/
int vaccel_write_nonserial_arg(struct vaccel_arg *args, int idx, void *buf,
			       void *(*serializer)(void *, uint32_t *));

/* Delete any allocated memory in the arg-list structure*/
int vaccel_delete_args(struct vaccel_arg_list *args);
