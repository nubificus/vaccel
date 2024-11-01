// SPDX-License-Identifier: Apache-2.0

#define _POSIX_C_SOURCE 200809L

#include "error.h"
#include "fs.h"
#include "log.h"
#include "path.h"

#ifdef LIBCURL

#include <curl/curl.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>

bool net_path_is_url(const char *path)
{
	bool is_url = false;

	if (!path)
		return is_url;

	CURLU *curl = curl_url();
	if (!curl) {
		vaccel_warn("Failed to create CURLU handle");
		return is_url;
	}

	CURLUcode ret = curl_url_set(curl, CURLUPART_URL, path, 0);
	if (ret == CURLUE_OK)
		is_url = true;

	curl_url_cleanup(curl);

	return is_url;
}

bool net_path_exists(const char *path)
{
	bool exists = false;

	if (!path)
		return exists;

	CURL *curl = curl_easy_init();
	if (!curl) {
		vaccel_warn("Failed to initialize CURL");
		return exists;
	}

	curl_easy_setopt(curl, CURLOPT_URL, path);
	curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

	CURLcode res = curl_easy_perform(curl);
	if (res == CURLE_OK)
		exists = true;

	curl_easy_cleanup(curl);

	return exists;
}

struct progress_data {
	double last_runtime;
	bool is_complete;
	CURL *curl;
};

#define KB 1024
#define MB (1024 * KB)
#define GB (1024 * MB)

#define B_INTERVAL 0.5
#define KB_INTERVAL 1
#define MB_INTERVAL 5
#define GB_INTERVAL 10
#define DEFAULT_INTERVAL KB_INTERVAL

static double convert_bytes(double bytes, const char **unit)
{
	if (bytes < KB) {
		*unit = "B";
		return bytes;
	}
	if (bytes < MB) {
		*unit = "KB";
		return bytes / KB;
	}
	if (bytes < GB) {
		*unit = "MB";
		return bytes / MB;
	}
	*unit = "GB";
	return bytes / GB;
}

static double get_progress_interval(double bytes)
{
	if (bytes == 0)
		return DEFAULT_INTERVAL;
	if (bytes < KB)
		return B_INTERVAL;
	if (bytes < MB)
		return KB_INTERVAL;
	if (bytes < GB)
		return MB_INTERVAL;
	return GB_INTERVAL;
}

static int download_progress_callback(void *p, curl_off_t dltotal,
				      curl_off_t dlnow, curl_off_t ultotal,
				      curl_off_t ulnow)
{
	(void)ultotal;
	(void)ulnow;

	if (!p) {
		vaccel_error("CURL: Progress data pointer is NULL");
		return 1;
	}
	struct progress_data *progress = (struct progress_data *)p;

	double cur_time;
	CURLcode ret = curl_easy_getinfo(progress->curl, CURLINFO_TOTAL_TIME,
					 &cur_time);
	if (ret != CURLE_OK) {
		vaccel_error("CURL: Error retrieving time info: %s",
			     curl_easy_strerror(ret));
		return 1;
	}

	/* Print progress on interval or on 100% */
	double interval = get_progress_interval(dltotal);
	if ((cur_time - progress->last_runtime < interval &&
	     dlnow != dltotal) ||
	    dlnow == 0 || progress->is_complete)
		return 0;

	progress->last_runtime = cur_time;

	const char *unit_downloaded;
	double downloaded = convert_bytes(dlnow, &unit_downloaded);

	/* Make sure 100% is only printed once */
	if (dlnow == dltotal)
		progress->is_complete = true;

	/* If we don't now the total size just print the downloaded one */
	if (dltotal > 0) {
		const char *unit_total;
		double total = convert_bytes(dltotal, &unit_total);
		double percentage = (dlnow * 100.0) / dltotal;
		double speed = (cur_time > 0) ?
				       convert_bytes(dlnow / cur_time,
						     &unit_downloaded) :
				       0;

		vaccel_debug(
			"Downloaded: %.1f %s of %.1f %s (%.1f%%) | Speed: %.2f %s/sec",
			downloaded, unit_downloaded, total, unit_total,
			percentage, speed, unit_downloaded);
	} else {
		vaccel_debug("Downloaded: %.1f %s", downloaded,
			     unit_downloaded);
	}

	fflush(stdout);

	/* Return 0 to continue download; return non-zero to abort */
	return 0;
}

int net_file_download(const char *path, const char *download_path)
{
	if (!path || !download_path)
		return VACCEL_EINVAL;

	if (strlen(download_path) + 1 > PATH_MAX) {
		vaccel_error("Path %s name too long", path);
		return VACCEL_ENAMETOOLONG;
	}

	if (fs_path_is_file(download_path))
		return VACCEL_EEXIST;

	if (fs_path_exists(download_path)) {
		vaccel_error("Path %s exists but is not a file", path);
		return VACCEL_EINVAL;
	}

	CURL *curl = curl_easy_init();
	if (!curl)
		return VACCEL_ENOMEM;

	FILE *fp = fopen(download_path, "wb");
	if (!fp) {
		vaccel_error("Could not open file %s: %s", download_path,
			     strerror(errno));
		return VACCEL_EIO;
	}

	curl_easy_setopt(curl, CURLOPT_URL, path);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

	/* Enable progress reporting */
	struct progress_data progress = { .last_runtime = 0,
					  .is_complete = false,
					  .curl = curl };
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
	curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION,
			 download_progress_callback);
	curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &progress);

	vaccel_debug("Downloading %s", path);
	CURLcode ret = curl_easy_perform(curl);
	if (ret)
		vaccel_error("CURL: %s", curl_easy_strerror(ret));
	else
		vaccel_debug("Download completed successfully");

	fclose(fp);
	curl_easy_cleanup(curl);

	return (ret == CURLE_OK) ? VACCEL_OK : VACCEL_EREMOTEIO;
}

#else

bool net_path_is_url(const char *path)
{
	(void)path;
	return false;
}

bool net_path_exists(const char *path)
{
	(void)path;
	return false;
}

int net_file_download(const char *path, const char *download_path)
{
	(void)path;
	(void)download_path;
	return VACCEL_ENOTSUP;
}

#endif
