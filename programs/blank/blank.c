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

    while(1) {
        if (getkey() != 0) {
            print("You pressed a key!\n");
        }
    }
    return 0;
}