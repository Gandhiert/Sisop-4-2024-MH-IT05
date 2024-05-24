<div align=center>

# Laporan Pengerjaan - Praktikum Modul 4 Sistem Operasi

</div>


## Kelompok IT05 - MH
Fikri Aulia As Sa'adi - 5027231026

Aisha Ayya Ratiandari - 5027231056

Gandhi Ert Julio - 5027231081

## _Soal 1_

### Dikerjakan oleh Fikri Aulia A (5027231026)

Dalam soal pertama ini, diminta untuk membuat program yang dapat menjalankan task berupa:
- Memberi watermark pada beberapa foto berformat .jpeg
- Me _reverse_ isi dari file .txt yang ber prefix "test"
- Mengubah permission script.sh agar bisa diakses

### Project Structure

- portofolio/ - Direktori dasar untuk proyek ini.
- portofolio/gallery/ - Direktori yang berisi gambar JPEG asli.
- portofolio/wm/ - Direktori tempat menyimpan gambar yang sudah diberi watermark.
- portofolio/bahaya/ - Direktori yang berisi file teks uji dan sebuah skrip.
- portofolio/bahaya/script.sh - File skrip yang akan diatur izinnya.
- portofolio/reversed/ - Direktori tempat menyimpan file teks yang dibalik.
- portofolio/mountpoint/ - Direktori tempat filesystem FUSE akan dimount.

### Penyelesaian
```c
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
```

### Penjelasan
#### Header dan Konstanta
```c
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
```
- FUSE_USE_VERSION: Mendefinisikan versi FUSE yang akan digunakan.
- Header: Menyertakan header yang diperlukan untuk FUSE, input/output, manipulasi string, penanganan kesalahan, informasi file sistem, alokasi memori, dan fungsi sistem.
- Konstanta: Mendefinisikan konstanta untuk path direktori yang digunakan dalam proyek.

#### Fungsi watermark_images
```c
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
```
- Bertujuan untuk menambahkan watermark pada gambar JPEG yang ada di input_dir dan menyimpannya di output_dir.
- Membuat direktori output jika belum ada.
- Menjalankan perintah shell untuk memproses setiap gambar JPEG di direktori input, menambahkan watermark menggunakan ImageMagick, dan menyimpan hasilnya di direktori output.

#### Fungsi set_script_permissions
```c
void set_script_permissions(const char *script_path) {
    if (chmod(script_path, S_IRWXU) != 0) {
        perror("chmod failed");
        exit(EXIT_FAILURE);
    }
}
```
- Bertujuan untuk mengatur izin file skrip agar dapat dieksekusi oleh pengguna.
- Menggunakan chmod untuk mengatur izin file ke S_IRWXU (baca, tulis, dan eksekusi untuk pemilik).

#### Fungsi reverse_test_files
```c
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
```
- Bertujuan untuk membalik isi file teks uji dan menyimpan hasilnya di direktori output.
- Membuat direktori output jika belum ada.
- Menjalankan perintah shell untuk menemukan file dengan nama test-*.txt, membalik isinya, dan menyimpan hasilnya dalam file baru di direktori output.

#### Fungsi do_getattr
```c
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
```
- Bertujuan untuk menyediakan atribut file atau direktori untuk path yang diberikan.
- Mengatur atribut untuk direktori root (/), direktori /wm, dan direktori /reversed.
- Untuk file di bawah /wm dan /reversed, memeriksa keberadaan dan ukuran file yang bersangkutan di direktori dasar.

#### Fungsi do_readdir
```c
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
```
- Bertujuan untuk menyediakan daftar direktori untuk path yang diberikan.
- Menangani direktori root (/), direktori /wm, dan direktori /reversed.
- Mendaftar file di direktori /wm dan /reversed dengan mengeksekusi perintah shell dan membaca outputnya.

#### Fungsi do_open
```c
static int do_open(const char *path, struct fuse_file_info *fi) {
    printf("do_open: %s\n", path);
    if (strncmp(path, "/wm/", 4) == 0 || strncmp(path, "/reversed/", 10) == 0) {
        char full_path[512];
        if (strncmp(path, "/wm/", 4) == 0) {
            snprintf(full_path, sizeof(full_path), "%s/wm%s", base_path, path + 3); // Adjust path to base path
        } else {
            snprintf(full_path, sizeof(full_path), "%s/reversed%s", base_path, path + 9); // Adjust path to base path
        }
        FILE *file = fopen(full_path, "r");
        if (file) {
            fclose(file);
            return 0;
        }
    }
    return -ENOENT;
}
```
- Bertujuan untuk membuka file untuk path yang diberikan.
- Mengizinkan pembukaan file di bawah /wm dan /reversed jika file tersebut ada.

#### Fungsi do_read
```c
static int do_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    printf("do_read: %s\n", path);
    if (strncmp(path, "/wm/", 4) == 0 || strncmp(path, "/reversed/", 10) == 0) {
        char full_path[512];
        if (strncmp(path, "/wm/", 4) == 0) {
            snprintf(full_path, sizeof(full_path), "%s/wm%s", base_path, path + 3); // Adjust path to base path
        } else {
            snprintf(full_path, sizeof(full_path), "%s/reversed%s", base_path, path + 9); // Adjust path to base path
        }

        FILE *file = fopen(full_path, "r");
        if (file) {
            fseek(file, offset, SEEK_SET);
            size_t bytes_read = fread(buf, 1, size, file);
            fclose(file);
            return bytes_read;
        }
    }
    return -ENOENT;
}
```
- Bertujuan untuk membaca isi file untuk path yang diberikan.
- Membaca isi file di bawah /wm dan /reversed, memastikan pembacaan sesuai dengan ukuran file dan offset.

#### Fungsi main
```c
int main(int argc, char *argv[]) {
    printf("Creating mount point directory: %s\n", mountpoint_path);
    mkdir(mountpoint_path, 0755);

    printf("Watermarking images in %s and saving to %s\n", gallery_path, watermark_path);
    watermark_images(gallery_path, watermark_path, watermark_text);

    printf("Setting script permissions: %s\n", script_path);
    set_script_permissions(script_path);

    printf("Reversing test files in %s and saving to %s\n", test_folder, reversed_path);
    reverse_test_files(test_folder, reversed_path);

    struct fuse_operations operations = {
        .getattr = do_getattr,
        .readdir = do_readdir,
        .open = do_open,
        .read = do_read,
    };

    printf("Starting FUSE filesystem\n");
    return fuse_main(argc, argv, &operations, NULL);
}
```
- Bertujuan untuk titik masuk program, menginisialisasi direktori yang diperlukan, memproses file, dan menjalankan loop utama FUSE.
- Membuat direktori mount point.
- Memanggil fungsi watermark_images untuk menambahkan watermark pada gambar.
- Memanggil fungsi set_script_permissions untuk mengatur izin skrip.
- Memanggil fungsi reverse_test_files untuk membalik file uji.
- Menjalankan loop utama FUSE dengan operasi dan mount point yang diberikan.

### Dokumentasi
![WhatsApp Image 2024-05-25 at 00 17 05 (3)](https://github.com/Gandhiert/Sisop-4-2024-MH-IT05/assets/142889150/6ea7be08-f544-44f9-b065-0247204f0b41)
>Isi folder /portofolio

![WhatsApp Image 2024-05-25 at 00 17 06](https://github.com/Gandhiert/Sisop-4-2024-MH-IT05/assets/142889150/59cda5ac-d825-41cb-9be1-8bb6b2fcb00b)
>Menjalankan program inikaryakita.c sebagai fuse_fs

![WhatsApp Image 2024-05-25 at 00 17 06 (1)](https://github.com/Gandhiert/Sisop-4-2024-MH-IT05/assets/142889150/af43cb6f-2d9c-4b71-975f-8364fefc31a0)
>Isi folder /portofolio setelah program dijalankan

![WhatsApp Image 2024-05-25 at 00 21 59](https://github.com/Gandhiert/Sisop-4-2024-MH-IT05/assets/142889150/4172bf08-54c0-4a1c-992c-b851d5bac685)
>Salah satu foto yang ada di folder /gallery

![WhatsApp Image 2024-05-25 at 00 17 05 (4)](https://github.com/Gandhiert/Sisop-4-2024-MH-IT05/assets/142889150/a4a683dc-6bf4-4c50-8dcd-c053c573504d)
>Isi folder /wm setelah program dijalankan

![WhatsApp Image 2024-05-25 at 00 20 07](https://github.com/Gandhiert/Sisop-4-2024-MH-IT05/assets/142889150/92776174-acdd-48f9-af8f-ab5a0a4ce1cb)
>Isi file test-adfi.txt yang ada di folder /bahaya

![WhatsApp Image 2024-05-25 at 00 20 22](https://github.com/Gandhiert/Sisop-4-2024-MH-IT05/assets/142889150/aa7a60c1-8dcd-4b0e-a712-2e9780daa5fa)
>Isi file test-adfi_reversed.txt yang ada di folder /reversed

![WhatsApp Image 2024-05-25 at 00 26 52](https://github.com/Gandhiert/Sisop-4-2024-MH-IT05/assets/142889150/642e1003-18e9-4a8b-aaad-e7278e2b8189)
>Isi file script.sh yang ada di dalam folder /bahaya










