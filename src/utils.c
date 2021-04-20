#include "utils.h"
#include "error.h"

#include <unistd.h>
#include <errno.h>

int cleanup_rundir(const char *path)
{
	/* At the moment, just assume that the rundir is empty, i.e. all
	 * contents have been cleaned up by the code that owns it */
	if (rmdir(path))
		return errno;

	return VACCEL_OK;
}
