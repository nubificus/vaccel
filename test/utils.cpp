#include <string>

extern "C" {

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>

#include "utils.h"

char* abs_path(const char* root, const char* file)
{
    std::string abs_path = std::string(root) + '/' + std::string(file);
    return strdup(abs_path.c_str());
}

unsigned char* read_file_from_dir(const char* dir, const char* path,
    size_t* len)
{
    char fpath[1024];

    snprintf(fpath, 1024, "%s/%s", dir, path);
    unsigned char* ptr;
    int ret = read_file_mmap(fpath, (void **)&ptr, len);
    if (ret)
        fprintf(stderr, "Could not mmap %s", fpath);

    return ptr;
}

}
