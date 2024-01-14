#include "benos.h"
#include "stdlib.h"

int main(int argc, char** argv) {
    
    print("Hello how are you?\n");

    void* ptr = malloc(512);
    free(ptr);

    if (ptr != 0) {
        print("ptr is not null!\n");
    } else {
        print("ptr is null!\n");
    }

    while(1) {
        if (getkey() != 0) {
            print("You pressed a key!\n");
        }
    }
    return 0;
}