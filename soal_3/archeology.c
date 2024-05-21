#define FUSE_USE_VERSION 31
#define _FILE_OFFSET_BITS 64

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>

#define MAX_FILE_SIZE 10240 // 10kb

static const char *dirpath = "/home/masgan/VEGITO/relics";

static int xmp_getattr(const char *path, struct stat *stbuf) {
    int res;
    char fpath[1024];  // buffer yang lebih besar
    snprintf(fpath, sizeof(fpath), "%s%s", dirpath, path);
    res = lstat(fpath, stbuf);
    if (res == -1)
        return -errno;
    return 0;
}

static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi) {
    char fpath[1024];  // buffer yang lebih besar
    snprintf(fpath, sizeof(fpath), "%s%s", dirpath, path);
    
    DIR *dp;
    struct dirent *de;

    (void) offset;
    (void) fi;

    dp = opendir(fpath);
    if (dp == NULL)
        return -errno;

    while ((de = readdir(dp)) != NULL) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;

        char filename[256];
        strcpy(filename, de->d_name);

        // Remove file extension
        char *dot = strrchr(filename, '.');
        if (dot != NULL && isdigit(*(dot+1))) {
            *dot = '\0';
        }

        if (filler(buf, filename, &st, 0))
            break;
    }

    closedir(dp);
    return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi) {
    char fpath[1024];  // buffer yang lebih besar
    snprintf(fpath, sizeof(fpath), "%s%s", dirpath, path);

    int fd = open(fpath, O_RDONLY);
    if (fd == -1)
        return -errno;

    int res = pread(fd, buf, size, offset);
    if (res == -1)
        res = -errno;

    close(fd);
    return res;
}

static int xmp_mkdir(const char *path, mode_t mode) {
    char fpath[1024];  // buffer yang lebih besar
    snprintf(fpath, sizeof(fpath), "%s%s", dirpath, path);

    int res = mkdir(fpath, mode);
    if (res == -1)
        return -errno;

    return 0;
}

static int xmp_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    char fpath[1024];  // buffer yang lebih besar
    snprintf(fpath, sizeof(fpath), "%s%s", dirpath, path);

    int fd = creat(fpath, mode);
    if (fd == -1)
        return -errno;

    fi->fh = fd;
    return 0;
}

static int xmp_write(const char *path, const char *buf, size_t size,
                     off_t offset, struct fuse_file_info *fi) {
    char fpath[1024];  // buffer yang lebih besar
    snprintf(fpath, sizeof(fpath), "%s%s", dirpath, path);

    int fd = open(fpath, O_WRONLY);
    if (fd == -1)
        return -errno;

    int res = pwrite(fd, buf, size, offset);
    if (res == -1)
        res = -errno;

    close(fd);

    // Split file into chunks
    char chunkpath[1024];  // buffer yang lebih besar
    int chunknum = 0;
    off_t chunkoffset = 0;
    while (chunkoffset < size) {
        snprintf(chunkpath, sizeof(chunkpath), "%s.%03d", fpath, chunknum);

        int chunkfd = creat(chunkpath, 0644);
        if (chunkfd == -1)
            return -errno;

        int chunksize = size - chunkoffset;
        if (chunksize > MAX_FILE_SIZE)
            chunksize = MAX_FILE_SIZE;

        int chunkres = pwrite(chunkfd, buf + chunkoffset, chunksize, 0);
        if (chunkres == -1)
            chunkres = -errno;

        close(chunkfd);

        chunkoffset += chunksize;
        chunknum++;
    }

    return res;
}

static int xmp_unlink(const char *path) {
    char fpath[1024];  // buffer yang lebih besar
    snprintf(fpath, sizeof(fpath), "%s%s", dirpath, path);

    // Remove file chunks
    char chunkpath[1024];  // buffer yang lebih besar
    int chunknum = 0;
    snprintf(chunkpath, sizeof(chunkpath), "%s.%03d", fpath, chunknum);
    while (access(chunkpath, F_OK) != -1) {
        unlink(chunkpath);
        chunknum++;
        snprintf(chunkpath, sizeof(chunkpath), "%s.%03d", fpath, chunknum);
    }

    int res = unlink(fpath);
    if (res == -1)
        return -errno;

    return 0;
}

static int xmp_rename(const char *from, const char *to) {
    char ffrom[1024];  // buffer yang lebih besar
    char fto[1024];  // buffer yang lebih besar
    snprintf(ffrom, sizeof(ffrom), "%s%s", dirpath, from);
    snprintf(fto, sizeof(fto), "%s%s", dirpath, to);

    int res = rename(ffrom, fto);
    if (res == -1)
        return -errno;

    // Rename file chunks
    char chunkfrom[1024];  // buffer yang lebih besar
    char chunkto[1024];  // buffer yang lebih besar
    int chunknum = 0;
    snprintf(chunkfrom, sizeof(chunkfrom), "%s.%03d", ffrom, chunknum);
    snprintf(chunkto, sizeof(chunkto), "%s.%03d", fto, chunknum);
    while (access(chunkfrom, F_OK) != -1) {
        rename(chunkfrom, chunkto);
        chunknum++;
        snprintf(chunkfrom, sizeof(chunkfrom), "%s.%03d", ffrom, chunknum);
        snprintf(chunkto, sizeof(chunkto), "%s.%03d", fto, chunknum);
    }

    return 0;
}

static int xmp_truncate(const char *path, off_t size) {
    char fpath[1024];  // buffer yang lebih besar
    snprintf(fpath, sizeof(fpath), "%s%s", dirpath, path);

    int res = truncate(fpath, size);
    if (res == -1)
        return -errno;

    // Truncate file chunks
    char chunkpath[1024];  // buffer yang lebih besar
    int chunknum = 0;
    snprintf(chunkpath, sizeof(chunkpath), "%s.%03d", fpath, chunknum);
    while (access(chunkpath, F_OK) != -1) {
        truncate(chunkpath, 0);
        chunknum++;
        snprintf(chunkpath, sizeof(chunkpath), "%s.%03d", fpath, chunknum);
    }

    return 0;
}

static int xmp_open(const char *path, struct fuse_file_info *fi) {
    char fpath[1024];  // buffer yang lebih besar
    snprintf(fpath, sizeof(fpath), "%s%s", dirpath, path);

    int fd = open(fpath, fi->flags);
    if (fd == -1)
        return -errno;

    fi->fh = fd;
    return 0;
}

static int xmp_release(const char *path, struct fuse_file_info *fi) {
    (void) path;
    close(fi->fh);
    return 0;
}

static int xmp_fsync(const char *path, int isdatasync,
                     struct fuse_file_info *fi) {
    (void) path;
    (void) isdatasync;
    return fsync(fi->fh);
}

static struct fuse_operations xmp_oper = {
    .getattr    = xmp_getattr,
    .readdir    = xmp_readdir,
    .read       = xmp_read,
    .mkdir      = xmp_mkdir,
    .create     = xmp_create,
    .write      = xmp_write,
    .unlink     = xmp_unlink,
    .rename     = xmp_rename,
    .truncate   = xmp_truncate,
    .open       = xmp_open,
    .release    = xmp_release,
    .fsync      = xmp_fsync,
};

int main(int argc, char *argv[]) {
    umask(0);
    return fuse_main(argc, argv, &xmp_oper, NULL);
}
