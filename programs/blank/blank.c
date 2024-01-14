#include "benos.h"
#include "stdlib.h"
#include "../stdlib/src/stdio.h"
#include "../stdlib/src/string.h"

int main(int argc, char** argv) {
    
    char* ptr = malloc(20);
    strcpy(ptr, "Hello world!\n");

    print(ptr);
    free(ptr);

    ptr[0] = 'b';
    
    while(1) {
        
    }
    
    return 0;
}