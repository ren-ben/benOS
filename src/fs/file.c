#include "file.h"
#include "../memory/memory.h"
#include "../memory/heap/kheap.h"
#include "../status.h"
#include "../kernel.h"
#include "../config.h"
#include "../string/string.h"
#include "fat/fat16.h"
#include "../disk/disk.h"

struct filesystem* filesystems[BENOS_MAX_FILESYSTEMS];
struct file_descriptor* file_descriptors[BENOS_MAX_FILE_DESCRIPTORS];

static struct filesystem** fs_get_free_filesystem() {
    int i = 0;
    for (i = 0; i < BENOS_MAX_FILESYSTEMS; i++)
    {
        if (filesystems[i] == 0)
        {
            return &filesystems[i];
        }
    }

    return 0;
}

void fs_insert_filesystem(struct filesystem* filesystem) {
    struct filesystem** fs;
    fs = fs_get_free_filesystem();
    if (!fs)
    {
        print("Problem inserting filesystem"); 
        while(1) {}
    }

    *fs = filesystem;
}

static void fs_static_load() {
    fs_insert_filesystem(fat16_init());
}

void fs_load() {
    memset(filesystems, 0, sizeof(filesystems));
    fs_static_load();
}

void fs_init() {
    memset(file_descriptors, 0, sizeof(file_descriptors));
    fs_load();
}

static int file_new_descriptor(struct file_descriptor** desc_out) {
    int res = -ENOMEM;
    for (int i = 0; i < BENOS_MAX_FILE_DESCRIPTORS; i++) {
        if (file_descriptors[i] == 0) {
            struct file_descriptor* desc = kzalloc(sizeof(struct file_descriptor));

            // descriptors start at 1
            desc->index = i + 1;
            file_descriptors[i] = desc;
            *desc_out = desc;
            res = 0;
            break;
        }
    }

    return res;
}

static struct file_descriptor* file_get_descriptor(int fd) {
    if (fd <= 0 || fd >= BENOS_MAX_FILE_DESCRIPTORS) {
        return 0;
    }

    return file_descriptors[fd - 1];
}

struct filesystem* fs_resolve(struct disk* disk) {
    struct filesystem* fs = 0;
    for (int i = 0; i < BENOS_MAX_FILESYSTEMS; i++) {
        if (filesystems[i] != 0 && filesystems[i]->resolve(disk) == 0) {
            fs = filesystems[i];
            break;
        }
    }

    return fs;
}

// choosing a file mode
FILE_MODE file_get_mode_by_string(const char* str) {
    FILE_MODE mode = FILE_MODE_INVALID;

    if (strncmp(str, "r", 1) == 0) {
        mode = FILE_MODE_READ;
    } else if (strncmp(str, "w", 1) == 0) {
        mode = FILE_MODE_WRITE;
    } else if (strncmp(str, "a", 1) == 0) {
        mode = FILE_MODE_APPEND;
    }

    return mode;
}

//opening a file in c (locate correct filesystem, call open)
int fopen(const char* filename, const char* mode_str) {
    int res = 0;
    struct path_root* root_path = pathparser_parse(filename, NULL);
    if (!root_path) {
        res = -EINVARG;
        goto out;
    }

    // can't have just a root path 0:/
    if(!root_path->first) {
        res = -EINVARG;
        goto out;
    }

    // ensure disk we are reading from exists
    struct disk* disk = disk_get(root_path->drive_no);
    if (!disk) {
        res = -EIO;
        goto out;
    }

    if (!disk->filesystem) {
        res = -EIO;
        goto out;
    }

    FILE_MODE mode = file_get_mode_by_string(mode_str);
    if (mode == FILE_MODE_INVALID) {
        res = -EINVARG;
        goto out;
    }

    void* descriptor_private_data = disk->filesystem->open(disk, root_path->first, mode);
    if (ISERR(descriptor_private_data)) {
        res = ERROR_I(descriptor_private_data);
        goto out;
    }

    struct file_descriptor* desc = 0;
    res = file_new_descriptor(&desc);
    if (res < 0) {
        goto out;
    }
    desc->fs = disk->filesystem;
    desc->private_data = descriptor_private_data;
    desc->disk = disk;
    res = desc->index;

out:
    // fopen should always return a positive number
    if (res < 0) {
        res = 0;
    }
    return res;
}

int fstat(int fd, struct file_stat* stat) {
    int res = 0;
    struct file_descriptor* desc = file_get_descriptor(fd);
    if (!desc) {
        res = -EIO;
        goto out;
    }

    res = desc->fs->stat(desc->private_data, stat);

out:
    return res;
}

int fclose (int fd) {
    int res = 0;
    struct file_descriptor* desc = file_get_descriptor(fd);
    if (!desc) {
        res = -EIO;
        goto out;
    }

    res = desc->fs->close(desc->private_data);

out:
    return res;
}

int fseek(int fd, int offset, FILE_SEEK_MODE whence) {
    int res = 0;
    struct file_descriptor* desc = file_get_descriptor(fd);
    if (!desc) {
        res = -EIO;
        goto out;
    }

    res = desc->fs->seek(desc->private_data, offset, whence);

out:
    return res;

}

int fread(void* ptr, uint32_t size, uint32_t nmemt, int fd) {
    int res = 0;
    if (size == 0 || nmemt == 0) {
        res = -EINVARG;
        goto out;
    }

    struct file_descriptor* desc = file_get_descriptor(fd);
    if (!desc) {
        res = -EINVARG;
        goto out;
    }

    res = desc->fs->read(desc->disk, desc->private_data, size, nmemt, (char*) ptr);

out:
    return res;
}