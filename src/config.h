#ifndef CONFIG_H
#define CONFIG_H

#define KERNEL_CODE_SELECTOR 0x08
#define KERNEL_DATA_SELECTOR 0x10

#define BENOS_TOTAL_INTERRUPTS 512

// 100MB heap size
#define BENOS_HEAP_SIZE_BYTES 104857600
#define BENOS_HEAP_BLOCK_SIZE 4096
#define BENOS_HEAD_ADDRESS 0x01000000
#define BENOS_HEAP_TABLE_ADDRESS 0x00007E00

#define BENOS_SECTOR_SIZE 512

#endif