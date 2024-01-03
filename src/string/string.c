#include "string.h"
#include <stdbool.h>

int strlen(const char* ptr) {
    int i = 0;
    while(*ptr != 0) {
        i++;
        ptr++;
    }

    return i;
}

int strnlen(const char* ptr, int max) {
    int i = 0;
    for (i = 0; i < max; i++) {
        if(ptr[i] == 0) {
            break;
        }
    }

    return i;
}

bool isdigit(char c) {
    return c >= 48 && c <= 57;
}

// converts an ascii string to an integer
int tonum(char c) {
    return c - 48;
}