// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <errno.h>

/* vaccel return codes */
enum {
	VACCEL_OK = 0 /* All Good :D */
};
#define VACCEL_EINVAL EINVAL /* EINVAL: Invalid argument */
#define VACCEL_ENOMEM ENOMEM /* ENOMEM: Out of memory */
#define VACCEL_ENOTSUP ENOTSUP /* EOPNOTSUPP: Operation not supported */
#define VACCEL_EINPROGRESS \
	EINPROGRESS /* EINPROGRESS: Operation now in progress */
#define VACCEL_EBUSY EBUSY /* EBUSY: Device or resource busy */
#define VACCEL_EEXIST EEXIST /* EEXIST: File exists */
#define VACCEL_ENOENT ENOENT /* ENOENT:  No such file or directory */
#define VACCEL_ELIBBAD \
	ELIBBAD /* ELIBBAD: Accessing a corrupted shared library */
#define VACCEL_ENODEV ENODEV /* ENODEV: No such device */
#define VACCEL_EIO EIO /* EIO: I/O error */
#define VACCEL_ESESS ECONNRESET /* ECONNRESET: Connection reset by peer */
#define VACCEL_EBACKEND EPROTO /* EPROTO: Protocol error */
#define VACCEL_ENOEXEC ENOEXEC /* ENOEXEC: Exec format error */
#define VACCEL_ENAMETOOLONG ENAMETOOLONG /* ENAMETOOLONG: File name too long */
#define VACCEL_EUSERS EUSERS /* Too many users */
#define VACCEL_EPERM EPERM /* Operation not permitted */
