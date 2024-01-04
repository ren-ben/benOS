#ifndef DISK_H
#define DISK_H

#include "../fs/file.h"

typedef unsigned int BENOS_DISK_TYPE;

//stands for a real physical hard disk
#define BENOS_DISK_TYPE_REAL 0

struct disk {
    BENOS_DISK_TYPE type;
    int sector_size;

    // the disk id
    int id;

    struct filesystem* filesystem;

    // the private data of our fs
    void* fs_private;
};

struct disk* disk_get(int index);
void disk_search_and_init();
int disk_read_block(struct disk* idisk, unsigned int lba, int total, void* buffer);

#endif