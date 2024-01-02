#include "paging.h"
#include "../heap/kheap.h"
#include "../../status.h"

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

bool paging_is_alligned(void* addr) {
    return ((uint32_t)addr % PAGING_PAGE_SIZE) == 0;
}

int paging_get_indexes(void* v_address, uint32_t* dir_i_out, uint32_t* table_i_out) {
    int res = 0;
    if (!paging_is_alligned(v_address)) {
        res = -EINVARG;
        goto out;
    }

    //reminder: 1024 * 4096 = total number of bytes in the 4gb directory

    *dir_i_out = ((uint32_t)v_address / (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE));
    *table_i_out = ((uint32_t)v_address % (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE) / PAGING_PAGE_SIZE);
out:
    return res;
}

//set the value of a page table entry in a page directory
int paging_set(uint32_t* dir, void* virt, uint32_t val) {
    if (!paging_is_alligned(virt))
    {
        return -EINVARG;
    }

    uint32_t dir_i = 0;
    uint32_t table_i = 0;
    int res = paging_get_indexes(virt, &dir_i, &table_i);
    if (res < 0)
    {
        return res;
    }

    uint32_t entry = dir[dir_i];
    uint32_t* table = (uint32_t*)(entry & 0xFFFFF000);
    table[table_i] = val;

    return 0;
}