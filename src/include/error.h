// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <errno.h>

/* vaccel return codes */
enum {
	VACCEL_OK = 0 /* All Good :D */
};
#define VACCEL_EINVAL EINVAL /* Invalid argument */
#define VACCEL_ENOMEM ENOMEM /* Out of memory */
#define VACCEL_ENOTSUP ENOTSUP /* Operation not supported */
#define VACCEL_EINPROGRESS EINPROGRESS /* Operation now in progress */
#define VACCEL_EBUSY EBUSY /* Device or resource busy */
#define VACCEL_EEXIST EEXIST /* File exists */
#define VACCEL_ENOENT ENOENT /* No such file or directory */
#define VACCEL_ELIBBAD ELIBBAD /* Accessing a corrupted shared library */
#define VACCEL_ENODEV ENODEV /* No such device */
#define VACCEL_EIO EIO /* I/O error */
#define VACCEL_ESESS ECONNRESET /* Connection reset by peer */
#define VACCEL_EBACKEND EPROTO /* Protocol error */
#define VACCEL_ENOEXEC ENOEXEC /* Exec format error */
#define VACCEL_ENAMETOOLONG ENAMETOOLONG /* File name too long */
#define VACCEL_EUSERS EUSERS /* Too many users */
#define VACCEL_EPERM EPERM /* Operation not permitted */
#define VACCEL_ELOOP ELOOP /* Too many symbolic links encountered */
#define VACCEL_EMLINK EMLINK /* Too many links */
#define VACCEL_ENOSPC ENOSPC /* No space left on device */
#define VACCEL_ENOTDIR ENOTDIR /* Not a directory */
#define VACCEL_EROFS EROFS /* Read-only file system */
#define VACCEL_EACCES EACCES /* Permission denied */
#define VACCEL_EBADF EBADF /* Bad file number */
#define VACCEL_EREMOTEIO EREMOTEIO /* Remote I/O error */
