#include "benos.h"
#include "stdlib.h"
#include "../stdlib/src/stdio.h"

int main(int argc, char** argv) {
    
    printf("My age is %i\n", 98);
    print("Hello how are you?\n");

    print(itoa(8763));

    putchar('Z');


    void* ptr = malloc(512);
    free(ptr);

    char buf[1024];
    benos_terminal_readline(buf, 1024, true);

    print(buf);
    
    while(1) {
        
    }
    return 0;
}