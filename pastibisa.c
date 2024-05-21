#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>

#define MAX_PATH_LENGTH 1024
#define MAX_PASSWORD_LENGTH 100
#define LOG_FILE_PATH "/Users/rrrreins/sisop/nyoba4/logs-fuse.log"

static const char *sensitive_dir = "/Users/rrrreins/sisop/sensitive";
static const char *mountpoint = "/Users/rrrreins/sisop/nyoba4/nani";
static const char *rahasia_prefix = "rahasia";
static char rahasia_password[MAX_PASSWORD_LENGTH];

static void log_message(const char *status, const char *tag, const char *info) {
    FILE *log_file = fopen(LOG_FILE_PATH, "a");
    if (log_file == NULL) {
        fprintf(stderr, "Could not open log file: %s\n", LOG_FILE_PATH);
        return;
    }

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%d/%m/%Y-%H:%M:%S", t);

    fprintf(log_file, "[%s]::%s::[%s]::[%s]\n", status, timestamp, tag, info);
    fclose(log_file);
}

static int copy_file(const char *src, const char *dst) {
    int input, output;
    if ((input = open(src, O_RDONLY)) == -1) {
        log_message("FAILED", "copyFile", "Failed to open source file");
        return -errno;
    }
    if ((output = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1) {
        close(input);
        log_message("FAILED", "copyFile", "Failed to open destination file");
        return -errno;
    }

    char buffer[4096];
    ssize_t bytes;
    while ((bytes = read(input, buffer, sizeof(buffer))) > 0) {
        if (write(output, buffer, bytes) != bytes) {
            close(input);
            close(output);
            log_message("FAILED", "copyFile", "Failed to write to destination file");
            return -errno;
        }
    }

    close(input);
    close(output);

    log_message("SUCCESS", "copyFile", "File copied successfully");
    return 0;
}

static int copy_directory(const char *src, const char *dst) {
    struct stat st;
    struct dirent *de;
    DIR *dp;

    if (mkdir(dst, 0755) == -1) {
        if (errno != EEXIST) {
            log_message("FAILED", "copyDirectory", "Failed to create destination directory");
            return -errno;
        }
    }

    dp = opendir(src);
    if (dp == NULL) {
        log_message("FAILED", "copyDirectory", "Failed to open source directory");
        return -errno;
    }

    while ((de = readdir(dp)) != NULL) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) {
            continue;
        }

        char src_path[MAX_PATH_LENGTH];
        char dst_path[MAX_PATH_LENGTH];

        snprintf(src_path, sizeof(src_path), "%s/%s", src, de->d_name);
        snprintf(dst_path, sizeof(dst_path), "%s/%s", dst, de->d_name);

        if (stat(src_path, &st) == -1) {
            closedir(dp);
            log_message("FAILED", "copyDirectory", "Failed to get status of source file");
            return -errno;
        }

        if (S_ISDIR(st.st_mode)) {
            if (copy_directory(src_path, dst_path) != 0) {
                closedir(dp);
                log_message("FAILED", "copyDirectory", "Failed to copy subdirectory");
                return -errno;
            }
        } else {
            if (copy_file(src_path, dst_path) != 0) {
                closedir(dp);
                return -errno;
            }
        }
    }

    closedir(dp);
    log_message("SUCCESS", "copyDirectory", "Directory copied successfully");
    return 0;
}

static int copy_sensitive_files(const char *dir, const char *dst) {
    char src_pesan[MAX_PATH_LENGTH];
    char dst_pesan[MAX_PATH_LENGTH];
    char src_rahasia[MAX_PATH_LENGTH];
    char dst_rahasia[MAX_PATH_LENGTH];

    snprintf(src_pesan, sizeof(src_pesan), "%s/pesan", dir);
    snprintf(dst_pesan, sizeof(dst_pesan), "%s/pesan", dst);
    snprintf(src_rahasia, sizeof(src_rahasia), "%s/rahasia-berkas", dir);
    snprintf(dst_rahasia, sizeof(dst_rahasia), "%s/rahasia-berkas", dst);

    if (copy_directory(src_pesan, dst_pesan) != 0) {
        log_message("FAILED", "copySensitiveFiles", "Failed to copy 'pesan' directory");
        return -errno;
    }
    if (copy_directory(src_rahasia, dst_rahasia) !=0) {
        log_message("FAILED", "copySensitiveFiles", "Failed to copy 'rahasia-berkas' directory");
        return -errno;
    }

    log_message("SUCCESS", "copySensitiveFiles", "Sensitive files copied successfully");
    return 0;
}

static int rahasia_access(const char *path) {
    if (strncmp(path, "/rahasia-berkas/", strlen("/rahasia-berkas/")) == 0) {
        if (strlen(path) == strlen("/rahasia-berkas/") || strncmp(path + strlen("/rahasia-berkas/"), rahasia_prefix, strlen(rahasia_prefix)) == 0) {
            char input_password[MAX_PASSWORD_LENGTH];
            printf("Enter password: ");
            scanf("%s", input_password);
            if (strcmp(input_password, rahasia_password) != 0) {
                log_message("FAILED", "accessFile", "Incorrect password");
                return -EACCES;
            }
        }
    }
    return 0;
}

// Base64 decoding
static int base64_decode(const char *input, char *output) {
    BIO *bio, *b64;
    int decoded_size = 0;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new_mem_buf(input, -1);
    bio = BIO_push(b64, bio);

    decoded_size = BIO_read(bio, output, strlen(input));
    output[decoded_size] = '\0';

    BIO_free_all(bio);

    return decoded_size;
}

// ROT13 decoding
static void rot13_decode(char *input) {
    char *p = input;
    while (*p) {
        if (('a' <= *p && *p <= 'z')) {
            *p = ((*p - 'a' + 13) % 26) + 'a';
        } else if (('A' <= *p && *p <= 'Z')) {
            *p = ((*p - 'A' + 13) % 26) + 'A';
        }
        p++;
    }
}

// Hexadecimal decoding
static int hex_decode(const char *input, char *output) {
    int len = strlen(input);
    if (len % 2 != 0) return -1; // Jumlah karakter harus genap untuk mewakili byte

    int decoded_size = 0;
    for (int i = 0; i < len; i += 2) {
        char hex[3] = {input[i], input[i + 1], '\0'};
        int val;
        sscanf(hex, "%x", &val);
        output[decoded_size++] = (char)val;
    }
    output[decoded_size] = '\0';

    return decoded_size;
}

// Text reversal
static void reverse_text(char *str) {
    int len = strlen(str);
    for (int i = 0; i < len / 2; i++) {
        char temp = str[i];
        str[i] = str[len - i - 1];
        str[len - i - 1] = temp;
    }
}

static int decode_file(char *path) {
    char buf[MAX_PATH_LENGTH];
    strcpy(buf, path);
    char *prefix = strtok(buf, "_");
    char *filename = strtok(NULL, "_");

    char fpath[MAX_PATH_LENGTH];
    snprintf(fpath, sizeof(fpath), "%s/%s", sensitive_dir, path);

    FILE *file = fopen(fpath, "r+");
    if (!file) {
        log_message("FAILED", "decodeFile", "Failed to open file");
        return -errno;
    }

    char decoded_content[4096];
    size_t bytes_read = fread(decoded_content, 1, sizeof(decoded_content), file);
    fclose(file);

    if (bytes_read == 0) {
        log_message("FAILED", "decodeFile", "Failed to read file");
        return -errno;
    }

    if (strncmp(prefix, "base64", strlen("base64")) == 0) {
        // Decode Base64
        base64_decode(decoded_content, decoded_content);
    } else if (strncmp(prefix, "rot13", strlen("rot13")) == 0) {
        // Decode ROT13
        rot13_decode(decoded_content);
    } else if (strncmp(prefix, "hex", strlen("hex")) == 0) {
        // Decode from hexadecimal representation
        hex_decode(decoded_content, decoded_content);
    } else if (strncmp(prefix, "rev", strlen("rev")) == 0) {
        // Decode by reversing text
        reverse_text(decoded_content);
    } else {
        // No decoding required
        return 0;
    }

    FILE *decoded_file = fopen(fpath, "w");
    if (!decoded_file) {
        log_message("FAILED", "decodeFile", "Failed to open decoded file");
        return -errno;
    }

    size_t bytes_written = fwrite(decoded_content, 1, bytes_read, decoded_file);
    fclose(decoded_file);

    if (bytes_written != bytes_read) {
        log_message("FAILED", "decodeFile", "Failed to write decoded content to file");
        return -errno;
    }

    log_message("SUCCESS", "decodeFile", "File decoded successfully");
    return 0;
}

static int getattr_callback(const char *path, struct stat *stbuf) {
    int res = 0;
    memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else {
        char fpath[MAX_PATH_LENGTH];
        snprintf(fpath, sizeof(fpath), "%s%s", sensitive_dir, path);

        res = lstat(fpath, stbuf);
        if (res == -1) {
            return -errno;
        }
    }

    return res;
}

static int readdir_callback(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    (void) offset;
    (void) fi;

    char fpath[MAX_PATH_LENGTH];
    snprintf(fpath, sizeof(fpath), "%s%s", sensitive_dir, path);

    DIR *dp;
    struct dirent *de;

    dp = opendir(fpath);
    if (dp == NULL) {
        return -errno;
    }

    while ((de = readdir(dp)) != NULL) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        if (filler(buf, de->d_name, &st, 0)) {
            break;
        }
    }

    closedir(dp);
    return 0;
}

static int open_callback(const char *path, struct fuse_file_info *fi) {
    if (rahasia_access(path) != 0) {
        return -EACCES;
    }

    char fpath[MAX_PATH_LENGTH];
    strcpy(fpath, sensitive_dir);
    strcat(fpath, path);

    int res = open(fpath, fi->flags);
    if (res == -1) {
        return -errno;
    }

    close(res);
    return 0;
}

static int read_callback(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    char fpath[MAX_PATH_LENGTH];
    strcpy(fpath, sensitive_dir);
    strcat(fpath, path);

    int fd = open(fpath, O_RDONLY);
    if (fd == -1) {
        return -errno;
    }

    int res = pread(fd, buf, size, offset);
    if (res == -1) {
        res = -errno;
    }

    close(fd);
    return res;
}

static int write_callback(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    char fpath[MAX_PATH_LENGTH];
    strcpy(fpath, sensitive_dir);
    strcat(fpath, path);

    int fd = open(fpath, O_WRONLY);
    if (fd == -1) {
        return -errno;
    }

    int res = pwrite(fd, buf, size, offset);
    if (res == -1) {
        res = -errno;
    }

    close(fd);
    return res;
}

static struct fuse_operations ops = {
    .getattr    = getattr_callback,
    .readdir    = readdir_callback,
    .open       = open_callback,
    .read       = read_callback,
    .write      = write_callback,
};

int main(int argc, char *argv[]) {
    copy_sensitive_files(sensitive_dir, mountpoint);

    printf("Set password for accessing rahasia-berkas: ");
    scanf("%s", rahasia_password);

    return fuse_main(argc, argv, &ops);
}
