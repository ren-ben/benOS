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

void paging_free_4gb(struct paging_4gb_chunk* chunk) {
    for (int i = 0; i < 1024; i++) {
        uint32_t entry = chunk->directory_entry[i];
        uint32_t* table = (uint32_t*)(entry & 0xFFFFF000);
        kfree(table);
    }

    kfree(chunk->directory_entry);
    kfree(chunk);
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

int paging_map(uint32_t* dir, void* virt, void* phys, int flags) {

    // checking if the address is alligned
    if (((unsigned int)virt % PAGING_PAGE_SIZE) || ((unsigned int)phys % PAGING_PAGE_SIZE)) {
        return -EINVARG;
    }

    // getting the indexes of the page directory and page table
    return paging_set(dir, virt, (uint32_t)phys | flags);
}

int paging_map_range(uint32_t* dir, void* virt, void* phys, int count, int flags) {
    int res = 0;
    for (int i = 0; i < count; i++) {
        res = paging_map(dir, virt, phys, flags);
        if (res == 0) {
            break;
        }

        virt += PAGING_PAGE_SIZE;
        phys += PAGING_PAGE_SIZE;
    }
}


int paging_map_to(uint32_t* dir, void* virt, void* phys, void* phys_end, int flags) {
    int res = 0;
    if ((uint32_t)virt % PAGING_PAGE_SIZE != 0) {
        res = -EINVARG;
        goto out;
    }

    if ((uint32_t)phys % PAGING_PAGE_SIZE) {
        res = -EINVARG;
        goto out;
    }

    if ((uint32_t)phys_end % PAGING_PAGE_SIZE) {
        res = -EINVARG;
        goto out;
    }

    if ((uint32_t)phys_end <= (uint32_t)phys) {
        res = -EINVARG;
        goto out;
    }

    uint32_t total_bytes = phys_end - phys;
    int total_pages = total_bytes / PAGING_PAGE_SIZE;
    res = paging_map_range(dir, virt, phys, total_pages, flags);

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