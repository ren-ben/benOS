#include "benos.h"
#include "stdlib.h"
#include "../stdlib/src/stdio.h"
#include "../stdlib/src/string.h"

int main(int argc, char** argv) {
    
    for(int i = 0; i < argc; i++) {
        printf("%s\n", argv[i]);
    }
    return 0;
}