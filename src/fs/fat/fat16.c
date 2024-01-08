#include "fat16.h"
#include "../../string/string.h"
#include "../../status.h"
#include "disk/disk.h"
#include "disk/streamer.h"
#include "memory/memory.h"
#include "memory/heap/kheap.h"
#include "../kernel.h"
#include <stdint.h>

#define BENOS_FAT16_SIGNATURE 0x29
#define BENOS_FAT16_FAT_ENTRY_SIZE 0x02
#define BENOS_FAT16_BAD_SECTOR 0xFF7
#define BENOS_FAT16_UNUSED 0x00

typedef unsigned int FAT_ITEM_TYPE;
#define FAT_ITEM_TYPE_DIR 0
#define FAT_ITEM_TYPE_FILE 1

// fat dir entry attributes bitmask
#define FAT_FILE_READ_ONLY 0x01
#define FAT_FILE_HIDDEN 0x02
#define FAT_FILE_SYSTEM 0x04
#define FAT_FILE_VOLUME_LABEL 0x08
#define FAT_FILE_SUBDIRECTORY 0x10
#define FAT_FILE_ARCHIVED 0x20
#define FAT_FILE_DEVICE 0x40
#define FAT_FILE_RESERVED 0x80

struct fat_header_extended {
    uint8_t drive_number;
    uint8_t win_nt_bit;
    uint8_t signature;
    uint32_t volume_id;
    uint8_t volume_id_string[11];
    uint8_t system_id_string[8];
} __attribute__((packed));

struct fat_header {
    uint8_t short_jmp_ins[3];
    uint8_t oem_identifier[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t fat_copies;
    uint16_t root_dir_entries;
    uint16_t num_of_sectors;
    uint8_t media_type;
    uint16_t sectors_per_fat;
    uint16_t sectors_per_track;
    uint16_t num_of_heads;
    uint32_t hidden_sectors;
    uint32_t sectors_big;
} __attribute__((packed));

struct fat_h {
    struct fat_header primary_header;
    union hat_h_e {
        struct fat_header_extended extended_header;
    } shared;
};

// fat dir entry
struct fat_dir_item {
    uint8_t fname[8];
    uint8_t ext[3];
    uint8_t attributes;
    uint8_t reserved;
    uint8_t creation_time_ms;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access_date;
    uint16_t high_16_bits_first_cluster;
    uint16_t last_modification_time;
    uint16_t last_modification_date;
    uint16_t low_16_bits_first_cluster;
    uint32_t file_size;
} __attribute__((packed));

struct fat_dir {
    struct fat_dir_item* items;
    int total;
    int sector_pos;
    int ending_sector_pos;
};

struct fat_item {
    union {
        struct fat_dir_item* item;
        struct fat_dir* dir;
    };

    FAT_ITEM_TYPE type;
};

struct fat_file_descriptor {
    struct fat_item* item;
    uint32_t pos;
};

struct fat_private {
    struct fat_h header;
    struct fat_dir root_dir;

    // used to stram data clusters
    struct disk_stream* cluster_read_stream;

    //used to stream the file allocation table
    struct disk_stream* fat_read_stream;

    // used in situations where we stream the dir
    struct disk_stream* dir_stream;
};

int fat16_resolve(struct disk* disk);
void* fat16_open(struct disk* disk, struct path_part* path, FILE_MODE mode);
int fat16_read(struct disk* disk, void* descriptor, uint32_t size, uint32_t nmemb, char* out_ptr);
int fat16_seek(void* private, uint32_t offset, FILE_SEEK_MODE seek_mode);

struct filesystem fat16_fs = {
    .resolve = fat16_resolve,
    .open = fat16_open,
    .read = fat16_read,
    .seek = fat16_seek
};

struct filesystem* fat16_init() {
    strcpy(fat16_fs.name, "FAT16");
    return &fat16_fs;
};

static void fat16_init_private(struct disk* disk, struct fat_private* private) {
    memset(private, 0, sizeof(struct fat_private));
    private->cluster_read_stream = diskstreamer_new(disk->id);
    private->fat_read_stream = diskstreamer_new(disk->id);
    private->dir_stream = diskstreamer_new(disk->id);
}

int fat16_sector_to_abs(struct disk* disk, int sector) {
    //abs sector address e.g. s1=0, s2=512, s3=1024, etc.
    return sector * disk->sector_size;
}

int fat16_get_total_items_for_dir(struct disk* disk, uint32_t dir_start_sector) {
    struct fat_dir_item item;
    struct fat_dir_item empty_item;
    memset(&empty_item, 0, sizeof(empty_item));

    struct fat_private* fat_private = disk->fs_private;
    int res = 0;
    int i = 0;
    int dir_start_pos = dir_start_sector * disk->sector_size;

    struct disk_stream* stream = fat_private->dir_stream;
    if (diskstreamer_seek(stream, dir_start_pos) != BENOS_ALL_OK) {
        res = -EIO;
        goto out;
    }

    while (1) {
        if (diskstreamer_read(stream, &item, sizeof(item)) != BENOS_ALL_OK) {
            res = -EIO;
            goto out;
        }

        if (item.fname[0] == 0x00) {
            //done
            break;
        }

        if (item.fname[0] == 0xE5) {
            //skip (item unused)
            continue;
        }

        i++;
    }
    
    res = i;

out:
    return res;
}

int fat16_get_root_dir(struct disk* disk, struct fat_private* fat_private, struct fat_dir* dir) {
    int res = 0;
    struct fat_header* primary_header = &fat_private->header.primary_header;
    int root_dir_sector_pos = (primary_header->fat_copies * primary_header->sectors_per_fat) + primary_header->reserved_sectors;
    int root_dir_entries = fat_private->header.primary_header.root_dir_entries;
    int root_dir_size = (root_dir_entries * sizeof(struct fat_dir_item));
    int total_sectors = root_dir_size / disk->sector_size;

    // if the root dir size is not a multiple of the sector size, we need to add one more sector
    if (root_dir_size % disk->sector_size) {
        total_sectors += 1;
    }

    int total_items = fat16_get_total_items_for_dir(disk, root_dir_sector_pos);

    struct fat_dir_item* dir_item = kzalloc(root_dir_size);

    if (!dir_item) {
        res = -ENOMEM;
        goto out;
    }

    struct disk_stream* stream = fat_private->dir_stream;

    if(diskstreamer_seek(stream, fat16_sector_to_abs(disk, root_dir_sector_pos)) != BENOS_ALL_OK) {
        res = -EIO;
        goto out;
    }

    if(diskstreamer_read(stream, dir_item, root_dir_size) != BENOS_ALL_OK) {
        res = -EIO;
        goto out;
    }

    //everything is ok, set the dir
    dir->items = dir_item;
    dir->total = total_items;
    dir->sector_pos = root_dir_sector_pos;
    dir->ending_sector_pos = root_dir_sector_pos + (root_dir_size / disk->sector_size);


out:
    return res;
}

int fat16_resolve(struct disk* disk) {
    int res = 0;
    struct fat_private* fat_private = kzalloc(sizeof(struct fat_private));
    fat16_init_private(disk, fat_private);

    disk->fs_private = fat_private;
    //binded the filesystem to the disk
    disk->filesystem = &fat16_fs;

    struct disk_stream* stream = diskstreamer_new(disk->id);
    if (!stream) {
        res =  -ENOMEM;
        goto out;
    }

    if(diskstreamer_read(stream, &fat_private->header, sizeof(fat_private->header)) != BENOS_ALL_OK) {
        res = -EIO;
        goto out;
    }

    if(fat_private->header.shared.extended_header.signature != 0x29) {
        res = -EFSNOTUS;
        goto out;
    }

    if(fat16_get_root_dir(disk, fat_private, &fat_private->root_dir) != BENOS_ALL_OK) {
        res = -EIO;
        goto out;
    }


out:
    if (stream) {
        diskstreamer_close(stream);
    }
    if (res < 0) {
        kfree(fat_private);
        disk->fs_private = 0;
    }
    return res;
}

void fat16_to_proper_string(char** out, const char* in) {
    while(*in != 0x00 && *in != 0x20) {
        **out = *in;
        *out += 1;
        in += 1;
    }

    if (*in == 0x20) {
        **out = 0x00;
    }

}

// fills out with the full relative filename (e.g. "test.txt")
void fat16_get_full_relative_filename(struct fat_dir_item* item, char* out, int max_len) {
    memset(out, 0x00, max_len);
    char *out_tmp = out;
    fat16_to_proper_string(&out_tmp, (const char*) item->fname);
    if (item->ext[0] != 0x00 && item->ext[0] != 0x20)  {
        *out_tmp++ = '.';
        fat16_to_proper_string(&out_tmp, (const char*) item->ext);
    }

}

// clones a dir item
struct fat_dir_item* fat16_clone_dir_item(struct fat_dir_item* item, int size) {
    struct fat_dir_item* item_copy = 0;
    if (size < sizeof(struct fat_dir_item)) {
        return 0;
    }
    item_copy = kzalloc(size);
    if (!item_copy) {
        return 0;
    }

    memcpy(item_copy, item, size);

    return item_copy;
}

static uint32_t fat16_get_first_cluster(struct fat_dir_item* item) {
    return (item->high_16_bits_first_cluster) | item->low_16_bits_first_cluster;
}
static int fat16_cluster_to_sector(struct fat_private* private, int cluster) {
    return private->root_dir.ending_sector_pos + ((cluster - 2) * private->header.primary_header.sectors_per_cluster);
}

static uint32_t fat16_get_first_fat_sector(struct fat_private* private) {
    return private->header.primary_header.reserved_sectors;
}

static int fat16_get_fat_entry(struct disk* disk, int cluster) {
    int res = -1;
    struct fat_private* private = disk->fs_private;
    struct disk_stream* stream = private->fat_read_stream;
    if (!stream) {
        goto out;
    }

    uint32_t fat_table_position = fat16_get_first_fat_sector(private) * disk->sector_size;
    res = diskstreamer_seek(stream, fat_table_position * (cluster * BENOS_FAT16_FAT_ENTRY_SIZE));
    if (res < 0) {
        goto out;
    }

    uint16_t result = 0;
    res = diskstreamer_read(stream, &result, sizeof(result));
    if (res < 0) {
        goto out;
    }

    res = result;
out:
    return res;
}

// get correct cluster to use based on starting cluster & offset
static int fat16_get_cluster_for_offset(struct disk* disk, int starting_cluster, int offset) {
    int res = 0;
    struct fat_private* private = disk->fs_private;
    int size_of_cluster_bytes = private->header.primary_header.sectors_per_cluster * disk->sector_size;
    int cluster_to_use = starting_cluster;
    int clusters_ahead = offset / size_of_cluster_bytes;
    for (int i = 0; i < clusters_ahead; i++) {
        int entry = fat16_get_fat_entry(disk, cluster_to_use);
        if (entry == 0xFF8 || entry == 0xFFF) {
            // you are at the last entry of the file
            res = -EIO;
            goto out;
        }

        // check whether the sector is bad
        if (entry == BENOS_FAT16_BAD_SECTOR) {
            res = -EIO;
            goto out;
        }

        // reserved sectors
        if (entry == 0xFF0 || entry == 0xFF6) {
            res = -EIO;
            goto out;
        }

        if (entry == 0x00) {
            res = -EIO;
            goto out;
        }

        cluster_to_use = entry;
    }

    res = cluster_to_use;

out:
    return res;
}

static int fat16_read_internal_from_stream(struct disk* disk, struct disk_stream* stream, int cluster, int offset, int total, void* out) {
    int res = 0;
    struct fat_private* private = disk->fs_private;
    int size_of_cluster_bytes = private->header.primary_header.sectors_per_cluster * disk->sector_size;
    int cluster_to_use = fat16_get_cluster_for_offset(disk, cluster, offset);
    if (cluster_to_use < 0) {
        res = cluster_to_use;
        goto out;
    }

    int offset_from_cluster = offset % size_of_cluster_bytes;

    int starting_sector = fat16_cluster_to_sector(private, cluster_to_use);
    int starting_pos = (starting_sector * disk->sector_size) + offset_from_cluster;
    int total_to_read = total > size_of_cluster_bytes ? size_of_cluster_bytes : total;
    
    res = diskstreamer_seek(stream, starting_pos);
    if (res != BENOS_ALL_OK) {
        goto out;
    }

    res = diskstreamer_read(stream, out, total_to_read);
    if (res != BENOS_ALL_OK) {
        goto out;
    }

    total -= total_to_read;

    if (total > 0) {
        // still have more to read
        res = fat16_read_internal_from_stream(disk, stream, cluster, offset+total_to_read, total, out+total_to_read);
    }

out:
    return res;
}

static int fat16_read_internal(struct disk* disk, int start_cluster, int offset, int total, void* out) {
    struct fat_private* fs_private = disk->fs_private;
    struct disk_stream* stream = fs_private->cluster_read_stream;
    return fat16_read_internal_from_stream(disk, stream, start_cluster, offset, total, out);
}

// frees a dir
void fat16_free_dir(struct fat_dir* dir) {
    if (!dir) {
        return;
    }

    if (dir->items) {
        kfree(dir->items);
    }
    
    kfree(dir);
}

// frees a fat item
void fat16_fat_item_free(struct fat_item* item) {
    if (item->type == FAT_ITEM_TYPE_DIR) {
        fat16_free_dir(item->dir);
    } else if (item->type == FAT_ITEM_TYPE_FILE) {
        kfree(item->item);
    }

    kfree(item);
}

struct fat_dir* fat16_load_fat_dir(struct disk* disk, struct fat_dir_item* item) {
    int res = 0;
    struct fat_dir* dir = 0;
    struct fat_private* fat_private = disk->fs_private;
    if (!(item->attributes & FAT_FILE_SUBDIRECTORY)) {
        res = -EINVARG;
        goto out;
    }

    dir = kzalloc(sizeof(struct fat_dir));
    if (!dir) {
        res = -ENOMEM;
        goto out;
    }

    int cluster = fat16_get_first_cluster(item);
    int cluster_sector = fat16_cluster_to_sector(fat_private, cluster);
    int total_items = fat16_get_total_items_for_dir(disk, cluster_sector);
    dir->total = total_items;
    int dir_size = dir->total * sizeof(struct fat_dir_item);
    dir->items = kzalloc(dir_size);
    if(!dir->items) {
        res = -ENOMEM;
        goto out;
    }

    res = fat16_read_internal(disk, cluster, 0x00, dir_size, dir->items);
    if (res != BENOS_ALL_OK) {
        goto out;
    }

out:
    if (res != BENOS_ALL_OK) {
        fat16_free_dir(dir);
    }
    return dir;
}

struct fat_item* fat16_new_fat_item_for_dir_item(struct disk* disk, struct fat_dir_item* item) {
    struct fat_item* f_item = kzalloc(sizeof(struct fat_item));
    if (!f_item) {
        return 0;
    }

    if (item->attributes & FAT_FILE_SUBDIRECTORY) {
        f_item->dir = fat16_load_fat_dir(disk, item);
        f_item->type = FAT_ITEM_TYPE_DIR;
    }

    f_item->type = FAT_ITEM_TYPE_FILE;
    f_item->item = fat16_clone_dir_item(item, sizeof(struct fat_dir_item));
    return f_item;
}

struct fat_item* fat16_find_item_in_dir(struct disk* disk, struct fat_dir* dir, const char* name) {
    struct fat_item* f_item = 0;
    char tmp_filename[BENOS_MAX_PATH];
    for (int i = 0; i < dir->total; i++) {
        fat16_get_full_relative_filename(&dir->items[i], tmp_filename, sizeof(tmp_filename));
        if (istrncmp(tmp_filename, name, sizeof(tmp_filename)) == 0) {
            f_item = fat16_new_fat_item_for_dir_item(disk, &dir->items[i]);
        }
    }

    return f_item;
}

struct fat_item* fat16_get_dir_entry(struct disk* disk, struct path_part* path) {
    struct fat_private* fat_private = disk->fs_private;
    struct fat_item* curr_item = 0;
    struct fat_item* root_item = fat16_find_item_in_dir(disk, &fat_private->root_dir, path->part);

    if (!root_item) {
        goto out;
    }

    struct path_part* next_part = path->next;
    curr_item = root_item;
    while (next_part != 0) {
        if (curr_item->type != FAT_ITEM_TYPE_DIR) {
            curr_item = 0;
            break;
        }

        struct fat_item* tmp_item = fat16_find_item_in_dir(disk, curr_item->dir, next_part->part);
        fat16_fat_item_free(curr_item);
        curr_item = tmp_item;
        next_part = next_part->next;
    }

out:
    return curr_item;
}

void* fat16_open(struct disk* disk, struct path_part* path, FILE_MODE mode) {
    if (mode != FILE_MODE_READ) {
        return ERROR(-EREADONLY);
    }

    struct fat_file_descriptor* descriptor = 0;
    descriptor = kzalloc(sizeof(struct fat_file_descriptor));
    if (!descriptor) {
        return ERROR(-ENOMEM);
    }

    descriptor->item = fat16_get_dir_entry(disk, path);
    if(!descriptor->item) {
        return ERROR(-EIO);
    }

    // files start at 0
    descriptor->pos = 0;
    return descriptor;
}

int fat16_read(struct disk* disk, void* descriptor, uint32_t size, uint32_t nmemb, char* out_ptr) {
    int res = 0;
    struct fat_file_descriptor* fat_desc = descriptor;
    struct fat_dir_item* item = fat_desc->item->item;
    int offset = fat_desc->pos;
    for(uint32_t i = 0; i < nmemb; i++) {
        res = fat16_read_internal(disk, fat16_get_first_cluster(item), offset, size, out_ptr);
        if (ISERR(res)) {
            goto out;
        }        

        out_ptr += size;
        offset += size;
    }
    res = nmemb;

out:
    return res;
}

int fat16_seek(void* private, uint32_t offset, FILE_SEEK_MODE seek_mode) {
    int res = 0;
    struct fat_file_descriptor* desc = private;
    struct fat_item* desc_item = desc->item;
    if (desc_item->type != FAT_ITEM_TYPE_FILE) {
        res = -EINVARG;
        goto out;
    }

    struct fat_dir_item* ritem = desc_item->item;
    if(offset >= ritem->file_size) {
        res = -EIO;
        goto out;
    }

    switch (seek_mode)
    {
    case SEEK_SET:
        desc->pos = offset;
        break;
    
    case SEEK_END:
        res = -EUNIMP;
        break;
    
    case SEEK_CUR:
        desc->pos += offset;
        break;
    
    default:
        res = -EINVARG;
        break;
    }

out:
    return res;
}