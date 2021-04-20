#ifndef __VACCEL_ERROR_H__
#define __VACCEL_ERROR_H__

/* vaccel return codes */
#define VACCEL_OK          0   /* All Good :D */
#define VACCEL_EINVAL      22  /* EINVAL: Invalid argument */
#define VACCEL_ENOMEM      13  /* ENOMEM: Out of memory */
#define VACCEL_ENOTSUP     95  /* EOPNOTSUPP: Operation not supported */
#define VACCEL_EINPROGRESS 115 /* EINPROGRESS: Operation now in progress */
#define VACCEL_EBUSY       16  /* EBUSY: Device or resource busy */
#define VACCEL_EEXISTS     17  /* EEXIST: File exists */
#define VACCEL_ENOENT      2   /* ENOENT:  No such file or directory */
#define VACCEL_ELIBBAD     80  /* ELIBBAD: Accessing a corrupted shared library */
#define VACCEL_ENODEV      19  /* ENODEV: No such device */
#define VACCEL_EIO         5   /* EIO: I/O error */
#define VACCEL_ESESS       104 /* ECONNRESET: Connection reset by peer */
#define VACCEL_EBACKEND    71  /* EPROTO: Protocol error */
#define VACCEL_ENOEXEC	   8   /* ENOEXEC: Exec format error */

#endif /* __VACCEL_ERROR_H__ */
