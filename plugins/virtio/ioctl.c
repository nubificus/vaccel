#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stddef.h>

#include <vaccel.h>

#include "log.h"

int dev_write(unsigned long req, void *data)
{
	int fd = open("/dev/accel", O_RDWR, 0);
	if (fd < 0)
		return VACCEL_ENODEV;


	int ret = ioctl(fd, req, data);
	if (ret)
		return VACCEL_EIO;

	return VACCEL_OK;
}
