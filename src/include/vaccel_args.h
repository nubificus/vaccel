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

#ifndef __INCLUDE_VACCEL_ARGS_H__
#define __INCLUDE_VACCEL_ARGS_H__

#include <stddef.h>
#include <stdint.h>

#include "error.h"

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
struct vaccel_arg_list* vaccel_args_init(uint32_t size);

/* Add a serialized argument in the list */
int vaccel_add_serial_arg(
    struct vaccel_arg_list* args,
    void* buf,
    uint32_t size
);

/* Add a non-serialized argument in the list */
int vaccel_add_nonserial_arg(
    struct vaccel_arg_list* args, 
    void* buf, 
    uint32_t argtype,
    void* (*serializer)(void*, uint32_t*)
);

/* Define an expected argument (serialized) */
int vaccel_expect_serial_arg(
    struct vaccel_arg_list* args,
    void* buf, 
    uint32_t size
);

/* Define an expected non-serialized argument */
int vaccel_expect_nonserial_arg(
    struct vaccel_arg_list* args,
    uint32_t expected_size
);

/* Extract a serialized argument out of the list */
void* vaccel_extract_serial_arg(
    struct vaccel_arg* args, 
    int idx
);

/* Extract a non-serialized argument out of the list */
void* vaccel_extract_nonserial_arg(
    struct vaccel_arg* args, 
    int idx, 
    void* (*deserializer)(void*, uint32_t)
);

/* 
Write to an expected serialized argument.
Used inside plugin. 
*/
int vaccel_write_serial_arg(
    struct vaccel_arg* args,
    int idx,
    void* buf
);

/* 
Write to an expected non-serialized argument.
Used inside plugin. 
*/
int vaccel_write_nonserial_arg(
    struct vaccel_arg* args,
    int idx,
    void* buf,
    void* (*serializer)(void*, uint32_t*)
);


/* Delete any allocated memory in the arg-list structure*/
int vaccel_delete_args(struct vaccel_arg_list* args);

#endif