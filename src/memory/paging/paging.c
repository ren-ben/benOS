#include "paging.h"
#include "../heap/kheap.h"

void paging_load_directory(uint32_t* dir);
static uint32_t* curr_dir = 0;
struct paging_4gb_chunk* paging_new_4gb(uint8_t flags) {

    uint32_t* dir = kzalloc(sizeof(uint32_t) * PAGING_TOTAL_ENTRIES_PER_TABLE);
    int offset = 0;
    for(int i = 0; i < PAGING_TOTAL_ENTRIES_PER_TABLE; i++) {
        uint32_t* entry = kzalloc(sizeof(uint32_t) * PAGING_TOTAL_ENTRIES_PER_TABLE);
        for (int b = 0; b < PAGING_TOTAL_ENTRIES_PER_TABLE; b++) {
            entry[b] = (offset + (b * PAGING_PAGE_SIZE)) | flags;
        }

        offset += (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE);
        dir[i] = (uint32_t)entry | flags | PAGING_IS_WRITABLE;
    }

    struct paging_4gb_chunk* chunk = kzalloc(sizeof(struct paging_4gb_chunk));
    chunk->directory_entry = dir;
    return chunk;
    //created a page directory with page tables that cover the entire 4gb of ram
}

void paging_switch(uint32_t* directory) {
    paging_load_directory(directory);
    curr_dir = directory;
}

uint32_t* paging_4g_chunk_get_dir(struct paging_4gb_chunk* chunk) {
    return chunk->directory_entry;
}