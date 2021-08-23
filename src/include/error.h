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

#ifndef __VACCEL_ERROR_H__
#define __VACCEL_ERROR_H__

#include <errno.h>

/* vaccel return codes */
#define VACCEL_OK           0             /* All Good :D */
#define VACCEL_EINVAL       EINVAL        /* EINVAL: Invalid argument */
#define VACCEL_ENOMEM       ENOMEM        /* ENOMEM: Out of memory */
#define VACCEL_ENOTSUP      ENOTSUP       /* EOPNOTSUPP: Operation not supported */
#define VACCEL_EINPROGRESS  EINPROGRESS   /* EINPROGRESS: Operation now in progress */
#define VACCEL_EBUSY        EBUSY         /* EBUSY: Device or resource busy */
#define VACCEL_EEXISTS      EEXIST        /* EEXIST: File exists */
#define VACCEL_ENOENT       ENOENT        /* ENOENT:  No such file or directory */
#define VACCEL_ELIBBAD      ELIBBAD       /* ELIBBAD: Accessing a corrupted shared library */
#define VACCEL_ENODEV       ENODEV        /* ENODEV: No such device */
#define VACCEL_EIO          EIO           /* EIO: I/O error */
#define VACCEL_ESESS        ECONNRESET    /* ECONNRESET: Connection reset by peer */
#define VACCEL_EBACKEND     EPROTO        /* EPROTO: Protocol error */
#define VACCEL_ENOEXEC      ENOEXEC       /* ENOEXEC: Exec format error */
#define VACCEL_ENAMETOOLONG ENAMETOOLONG  /* ENAMETOOLONG: File name too long */
#define VACCEL_EUSERS       EUSERS        /* Too many users */
#define VACCEL_EPERM        EPERM         /* Operation not permitted */

#endif /* __VACCEL_ERROR_H__ */
