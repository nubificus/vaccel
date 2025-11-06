// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <asm-generic/errno.h>
#include <errno.h>
#include <linux/errno.h>

/* vaccel return codes */
enum {
	VACCEL_OK = 0, /* All Good :D */
	VACCEL_EINVAL = EINVAL, /* Invalid argument */
	VACCEL_ENOMEM = ENOMEM, /* Out of memory */
	VACCEL_ENOTSUP = ENOTSUP, /* Operation not supported */
	VACCEL_EINPROGRESS = EINPROGRESS, /* Operation now in progress */
	VACCEL_EBUSY = EBUSY, /* Device or resource busy */
	VACCEL_EEXIST = EEXIST, /* File exists */
	VACCEL_ENOENT = ENOENT, /* No such file or directory */
	VACCEL_ELIBBAD = ELIBBAD, /* Accessing a corrupted shared library */
	VACCEL_ENODEV = ENODEV, /* No such device */
	VACCEL_EIO = EIO, /* I/O error */
	VACCEL_ESESS = ECONNRESET, /* Connection reset by peer */
	VACCEL_EBACKEND = EPROTO, /* Protocol error */
	VACCEL_ENOEXEC = ENOEXEC, /* Exec format error */
	VACCEL_ENAMETOOLONG = ENAMETOOLONG, /* File name too long */
	VACCEL_EUSERS = EUSERS, /* Too many users */
	VACCEL_EPERM = EPERM, /* Operation not permitted */
	VACCEL_ELOOP = ELOOP, /* Too many symbolic links encountered */
	VACCEL_EMLINK = EMLINK, /* Too many links */
	VACCEL_ENOSPC = ENOSPC, /* No space left on device */
	VACCEL_ENOTDIR = ENOTDIR, /* Not a directory */
	VACCEL_EROFS = EROFS, /* Read-only file system */
	VACCEL_EACCES = EACCES, /* Permission denied */
	VACCEL_EBADF = EBADF, /* Bad file number */
	VACCEL_EREMOTEIO = EREMOTEIO, /* Remote I/O error */
	VACCEL_EFAULT = EFAULT, /* Bad address */
	VACCEL_ERANGE = ERANGE, /* Out of range */
	VACCEL_EAGAIN = EAGAIN, /* Resource temporarily unavailable */
	VACCEL_ENOSYS = ENOSYS, /* Function not implemented */
};
