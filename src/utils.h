#ifndef __UTILS_H__
#define __UTILS_H__

/* It tries to cleanup a rundir created by vaccelrt. It will
 * return VACCEL_OK at success, or a proper error code otherwise
 */
int cleanup_rundir(const char *path);

#endif /* __UTILS_H__ */
