#include "stdlib.h"
#include "benos.h"

void* malloc(size_t size) {
    return benos_malloc(size);
}
void free(void* ptr) {
    benos_free(ptr);
}