#define FUSE_USE_VERSION 28
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>

static const char *base_path = "portofolio";
static const char *gallery_path = "portofolio/gallery";
static const char *watermark_path = "portofolio/wm";
static const char *script_path = "portofolio/bahaya/script.sh";
static const char *test_folder = "portofolio/bahaya";
static const char *reversed_path = "portofolio/reversed";
static const char *mountpoint_path = "portofolio/mountpoint";
static const char *watermark_text = "inikaryakita.id";

// Function to watermark images using ImageMagick
void watermark_images(const char *input_dir, const char *output_dir, const char *watermark_text) {
    char command[512];
    snprintf(command, sizeof(command), "mkdir -p %s", output_dir);
    system(command);

    snprintf(command, sizeof(command), "for img in %s/*.jpeg; do "
                                       "filename=$(basename \"$img\"); "
                                       "height=$(identify -format %%h \"$img\"); "
                                       "fontsize=$(($height / 40)); "
                                       "convert \"$img\" -gravity south -fill white -pointsize $fontsize -annotate +0+10 \"%s\" \"%s/$filename\"; "
                                       "echo \"Watermarked $img to %s/$filename\"; "
                                       "done", input_dir, watermark_text, output_dir, output_dir);
    printf("Executing command: %s\n", command);
    system(command);
}

// Function to set script permissions
void set_script_permissions(const char *script_path) {
    if (chmod(script_path, S_IRWXU) != 0) {
        perror("chmod failed");
        exit(EXIT_FAILURE);
    }
}

// Function to reverse the content of test files and create new files
void reverse_test_files(const char *folder_path, const char *output_dir) {
    char command[512];
    snprintf(command, sizeof(command), "mkdir -p %s", output_dir);
    system(command);

    snprintf(command, sizeof(command), "find %s -name 'test-*.txt' -exec sh -c 'for file; do "
                                       "filename=$(basename \"$file\"); "
                                       "rev \"$file\" > \"%s/${filename%%.txt}_reversed.txt\" && echo \"Reversed $file to %s/${filename%%.txt}_reversed.txt\"; "
                                       "done' sh {} +", folder_path, output_dir, output_dir);
    printf("Executing command: %s\n", command);
    system(command);
}

static int do_getattr(const char *path, struct stat *st) {
    memset(st, 0, sizeof(struct stat));
    printf("do_getattr: %s\n", path);
    if (strcmp(path, "/") == 0) {
        st->st_mode = S_IFDIR | 0755;
        st->st_nlink = 2;
    } else if (strcmp(path, "/wm") == 0) {
        st->st_mode = S_IFDIR | 0755;
        st->st_nlink = 2;
    } else if (strcmp(path, "/reversed") == 0) {
        st->st_mode = S_IFDIR | 0755;
        st->st_nlink = 2;
    } else if (strncmp(path, "/wm/", 4) == 0) {
        st->st_mode = S_IFREG | 0644;
        st->st_nlink = 1;
        char full_path[512];
        snprintf(full_path, sizeof(full_path), "%s/wm%s", base_path, path + 3); // Adjust path to base path
        printf("Checking file: %s\n", full_path);
        FILE *file = fopen(full_path, "r");
        if (file) {
            fseek(file, 0, SEEK_END);
            st->st_size = ftell(file);
            fclose(file);
        } else {
            printf("File not found: %s\n", full_path);
            return -ENOENT;
        }
    } else if (strncmp(path, "/reversed/", 10) == 0) {
        st->st_mode = S_IFREG | 0644;
        st->st_nlink = 1;
        char full_path[512];
        snprintf(full_path, sizeof(full_path), "%s/reversed%s", base_path, path + 9); // Adjust path to base path
        printf("Checking file: %s\n", full_path);
        FILE *file = fopen(full_path, "r");
        if (file) {
            fseek(file, 0, SEEK_END);
            st->st_size = ftell(file);
            fclose(file);
        } else {
            printf("File not found: %s\n", full_path);
            return -ENOENT;
        }
    } else {
        return -ENOENT;
    }
    return 0;
}

static int do_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    (void) offset;
    (void) fi;

    printf("do_readdir: %s\n", path);
    if (strcmp(path, "/") == 0) {
        filler(buf, ".", NULL, 0);
        filler(buf, "..", NULL, 0);
        filler(buf, "wm", NULL, 0);
        filler(buf, "reversed", NULL, 0);
    } else if (strcmp(path, "/wm") == 0) {
        filler(buf, ".", NULL, 0);
        filler(buf, "..", NULL, 0);

        printf("Reading directory: %s\n", "portofolio/wm");
        system("ls portofolio/wm > wm_files.txt");
        FILE *fp = fopen("wm_files.txt", "r");
        if (fp == NULL) return -errno;

        char filename[256];
        while (fscanf(fp, "%255s", filename) == 1) {
            printf("Adding wm file: %s\n", filename);
            filler(buf, filename, NULL, 0);
        }
        fclose(fp);
        system("rm wm_files.txt");
    } else if (strcmp(path, "/reversed") == 0) {
        filler(buf, ".", NULL, 0);
        filler(buf, "..", NULL, 0);

        printf("Reading directory: %s\n", "portofolio/reversed");
        system("ls portofolio/reversed > reversed_files.txt");
        FILE *fp = fopen("reversed_files.txt", "r");
        if (fp == NULL) return -errno;

        char filename[256];
        while (fscanf(fp, "%255s", filename) == 1) {
            printf("Adding reversed file: %s\n", filename);
            filler(buf, filename, NULL, 0);
        }
        fclose(fp);
        system("rm reversed_files.txt");
    } else {
        return -ENOENT;
    }

    return 0;
}

static int do_open(const char *path, struct fuse_file_info *fi) {
    printf("do_open: %s\n", path);
    if (strncmp(path, "/wm/", 4) == 0 || strncmp(path, "/reversed/", 10) == 0) {
        return 0;
    }
    return -ENOENT;
}

static int do_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    size_t len;
    (void) fi;
    printf("do_read: %s\n", path);
    if (strncmp(path, "/wm/", 4) == 0) {
        char full_path[512];
        snprintf(full_path, sizeof(full_path), "%s/wm%s", base_path, path + 3); // Adjust path to base path
        printf("Reading file: %s\n", full_path);
        FILE *file = fopen(full_path, "r");
        if (!file) {
            perror("fopen failed");
            return -ENOENT;
        }
        fseek(file, 0, SEEK_END);
        len = ftell(file);
        fseek(file, 0, SEEK_SET);
        if (offset < len) {
            if (offset + size > len)
                size = len - offset;
            fread(buf, 1, size, file);
        } else {
            size = 0;
        }
        fclose(file);
        return size;
    } else if (strncmp(path, "/reversed/", 10) == 0) {
        char full_path[512];
        snprintf(full_path, sizeof(full_path), "%s/reversed%s", base_path, path + 9); // Adjust path to base path
        printf("Reading file: %s\n", full_path);
        FILE *file = fopen(full_path, "r");
        if (!file) {
            perror("fopen failed");
            return -ENOENT;
        }
        fseek(file, 0, SEEK_END);
        len = ftell(file);
        fseek(file, 0, SEEK_SET);
        if (offset < len) {
            if (offset + size > len)
                size = len - offset;
            fread(buf, 1, size, file);
        } else {
            size = 0;
        }
        fclose(file);
        return size;
    }
    return -ENOENT;
}

static struct fuse_operations operations = {
    .getattr = do_getattr,
    .readdir = do_readdir,
    .open = do_open,
    .read = do_read,
};

int main(int argc, char *argv[]) {
    // Create the mountpoint directory
    mkdir(mountpoint_path, 0755);

    // Watermark images
    watermark_images(gallery_path, watermark_path, watermark_text);

    // Set script permissions
    set_script_permissions(script_path);

    // Reverse test files
    reverse_test_files(test_folder, reversed_path);

    // Run FUSE main loop
    const char *fuse_argv[] = { argv[0], mountpoint_path };
    int fuse_argc = 2;
    return fuse_main(fuse_argc, (char **) fuse_argv, &operations, NULL);
}
