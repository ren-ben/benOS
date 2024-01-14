#include "benos.h"
#include "stdlib.h"

int main(int argc, char** argv) {
    
    print("Hello how are you?\n");

    void* ptr = malloc(512);

    if (ptr) {
        print("Malloc succeeded!\n");
    } else {
        print("Malloc failed!\n");
    }

    while(1) {
        if (getkey() != 0) {
            print("You pressed a key!\n");
        }
    }
    return 0;
}