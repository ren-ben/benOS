#include "../io/io.h"
#include "disk.h"
#include "memory/memory.h"
#include "../config.h"
#include "status.h"
#include "../kernel.h"

struct disk disk;

//reads a sector from the disk
int disk_read_sector(int lba, int total, void* buffer) {
    //wait for the disk to be ready
    outb(0x1F6, (lba >> 24) | 0xE0);
    //send the lba
    outb(0x1F2, total);
    outb(0x1F3, (unsigned char)(lba & 0xFF));
    outb(0x1F4, (unsigned char)((lba >> 8) & 0xFF));
    outb(0x1F5, (unsigned char)((lba >> 16) & 0xFF));
   
   unsigned short* ptr = (unsigned short*)buffer;
    for (int i = 0; i < total; i++) {
         //wait for the buffer to be ready (checking for a flag)
         char c = insb(0x1F7);
         while(!(c & 0x08)) {
             c = insb(0x1F7);
         }
         
        //read the data

        //copy from hard disk to memory
        for (int b = 0; b < 256; b++) {
            *ptr = insw(0x1F0);
            ptr++;
        }
    }
    return 0;
}

//searches for a disk and initializes it
void disk_search_and_init() {
    print("\nInitializing disk...");
    memset(&disk, 0, sizeof(disk));
    disk.type = BENOS_DISK_TYPE_REAL;
    disk.sector_size = BENOS_SECTOR_SIZE;
}

//gets a disk with a specific index
struct disk* disk_get(int index) {
    if (index == 0) {
        return &disk;
    }
    return 0;
}

int disk_read_block(struct disk* idisk, unsigned int lba, int total, void* buffer) {
    if (idisk != &disk) {
        return -EIO;
    }

    return disk_read_sector(lba, total, buffer);
}