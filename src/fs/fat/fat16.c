#include "fat16.h"
#include "../../string/string.h"
#include "../../status.h"
#include "disk/disk.h"
#include "disk/streamer.h"
#include "memory/memory.h"
#include "memory/heap/kheap.h"
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
    char volume_id_string[11];
    char system_id_string[8];
} __attribute__((packed));

struct fat_header {
    uint8_t short_jump_ins[3];
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

struct fat_item_descriptor {
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

struct filesystem fat16_fs = {
    .resolve = fat16_resolve,
    .open = fat16_open
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
    int root_dir_sector_pos = (primary_header->fat_copies + primary_header->sectors_per_fat) + primary_header->reserved_sectors;
    int root_dir_entries = fat_private->header.primary_header.root_dir_entries;
    int root_dir_size = root_dir_entries * sizeof(struct fat_dir_item);
    int total_sectors = root_dir_size / disk->sector_size;

    // if the root dir size is not a multiple of the sector size, we need to add one more sector
    if (root_dir_size % disk->sector_size) {
        total_sectors++;
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
        res =  -EIO;
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

void* fat16_open(struct disk* disk, struct path_part* path, FILE_MODE mode) {
    return 0;
}