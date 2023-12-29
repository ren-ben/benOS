#include "kheap.h"
#include "heap.h"
#include "../../config.h"
#include "../../kernel.h"

struct heap kernel_heap;
struct heap_table kernel_heap_table;

void kheap_init() {
    print("\nInitializing kernel heap");

    // create the heap table
    int total_table_entries = BENOS_HEAP_SIZE_BYTES / BENOS_HEAP_BLOCK_SIZE;
    kernel_heap_table.entries = (HEAP_BLOCK_TABLE_ENTRY*) BENOS_HEAP_TABLE_ADDRESS ;
    kernel_heap_table.total = total_table_entries;

    void* end = (void*) BENOS_HEAD_ADDRESS + BENOS_HEAP_SIZE_BYTES;

    // create the heap
    int res = heap_create(&kernel_heap, (void*) BENOS_HEAD_ADDRESS, end, &kernel_heap_table);

    if (res < 0) {
        print("\nFailed to create kernel heap");
    }
}

void* kmalloc(size_t size) {
    return heap_malloc(&kernel_heap, size);
}

void kfree(void* ptr) {
    heap_free(&kernel_heap, ptr);
}