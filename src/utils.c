#include "utils.h"
#include "error.h"

#include <dirent.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>

bool dir_exists(const char *path)
{
	DIR *dir = opendir(path);
	if (!dir)
		return false;

	closedir(dir);
	return true;
}

int cleanup_rundir(const char *path)
{
	/* At the moment, just assume that the rundir is empty, i.e. all
	 * contents have been cleaned up by the code that owns it */
	if (rmdir(path))
		return errno;

	return VACCEL_OK;
}
