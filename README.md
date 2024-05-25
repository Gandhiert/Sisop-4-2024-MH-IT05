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


## _Soal 2_


## _Soal 3_
### Dikerjakan Oleh Gandhi Ert Julio (5027231081)
Soal ini menginstruksikan kita untuk membuat file sistem yang dapat di-mount menggunakan FUSE . File sistem ini berfungsi sebagai lapisan virtual di atas sistem file yang ada dan menyediakan berbagai operasi file sistem seperti membaca, menulis, membuat, menghapus, dan mengganti nama file atau direktori. Berikut adalah rincian dari apa yang dilakukan oleh kode ini:

### archeology.c

```bash
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


```
Fungsi ini Mengatur versi FUSE yang akan digunakan dan Menggunakan offset 64-bit untuk mendukung file berukuran besar serta menyertakan header library yang ditentukan.

```bash
static const char *dirpath = "/home/masgan/VEGITO/relics";

```
Fungsi ini Menentukan direktori dasar tempat file sistem virtual akan dioperasikan.

```bash

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
```
implementasai fungsi ini untuk Mendapatkan atribut file atau direktori dan Membaca isi direktori dan mengisi buffer dengan daftar file dan direktori.


```bash
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

```
Lalu di baris kode ini memiliki implementasi logika fungsi untuk membaca file dan membuat direktori baru hingga membuat file baru.
 
```bash
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
```
DI baris ini fungsi menulis data ke dalam file dan membagi file menjadi beberapa bagian jika melebihi ukuran buffer maksimum, di fungsi kedua untuk menghapus file dan bagian-bagian file terkait dengan instruksi soal.
 
 ```bash
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
```
Di baris ini memiliki fungsi untuk memotong ukuran file dan bagian file terkait dan mengganti nama file atau direktori termasuk bagian file.


```bash
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


```
Pada baris kode ini Fungsi akan memanipulasi proses pembukaan dan penutupan file.

```bash
static int xmp_fsync(const char *path, int isdatasync,
                     struct fuse_file_info *fi) {
    (void) path;
    (void) isdatasync;
    return fsync(fi->fh);
}
```

Dan di implementasi fungsi file sistem terakhir ada sinkronisasi data file dengan media storage system.

```bash
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
```
Lalu di baris kali ini ada pendefinisikan operasi file sistem yang diimplementasikan untuk menangani berbagai operasi file sistem FUSE.

```bash
int main(int argc, char *argv[]) {
    umask(0);
    return fuse_main(argc, argv, &xmp_oper, NULL);
}
```
Dan di fungsi utama unntuk Mengatur `umask` untuk memastikan bahwa file dan direktori yang dibuat memiliki izin yang tepat dan Memanggil `fuse_main` untuk menjalankan file sistem FUSE dengan operasi yang didefinisikan.

![Screenshot 2024-05-23 181408](https://github.com/Gandhiert/BARU-NYOBA/assets/136203533/cffac3a5-9901-4dfb-8563-2eb92e093f38)

>Hasil dari keteika ingin melakukan listing, masih terdapat relics yang tergabung sebagian.









