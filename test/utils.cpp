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

char* abs_path(const char* root, const char* file)
{
    std::string abs_path = std::string(root) + '/' + std::string(file);
    return strdup(abs_path.c_str());
}

int read_file(const char* filename, char** file_buf, size_t* file_buf_size)
{
    int fd;
    long bytes = 0;

    fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("open: ");
        return 1;
    }

    struct stat info;
    fstat(fd, &info);
    fprintf(stdout, "Image size: %luB\n", info.st_size);

    char* buf = (char*)malloc(info.st_size);
    if (!buf) {
        fprintf(stderr, "Could not allocate memory for image\n");
        goto free_buff;
    }

    do {
        int ret = read(fd, buf, info.st_size);
        if (ret < 0) {
            perror("Error while reading image: ");
            goto close_file;
        }
        bytes += ret;
    } while (bytes < info.st_size);

    if (bytes < info.st_size) {
        fprintf(stderr, "Could not read image\n");
        goto close_file;
    }

    *file_buf = buf;
    *file_buf_size = info.st_size;
    close(fd);

    return 0;

close_file:
    close(fd);
free_buff:
    free(buf);
    return 1;
}

unsigned char* read_file_mmap(const char* path, size_t* len)
{
    struct stat stat;
    int status, fd;

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("Could not open file");
        return NULL;
    }

    status = fstat(fd, &stat);
    if (status < 0) {
        perror("Could not stat file");
        return NULL;
    }

    unsigned char* ptr = static_cast<unsigned char*>(
        mmap(NULL, stat.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0));
    close(fd);

    if (!ptr)
        return NULL;

    *len = stat.st_size;

    return ptr;
}

unsigned char* read_file_from_dir(const char* dir, const char* path,
    size_t* len)
{
    char fpath[1024];

    snprintf(fpath, 1024, "%s/%s", dir, path);
    unsigned char* ptr = read_file_mmap(fpath, len);
    if (!ptr)
        fprintf(stderr, "Could not mmap %s", fpath);

    return ptr;
}

}
