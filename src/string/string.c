#include "string.h"
#include <stdbool.h>

char tolower(char c) {
    if (c >= 65 && c <= 90) {
        return c + 32;
    }

    return c;
}

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

int strncmp(const char* str1, const char* str2, int len) {
    unsigned char u1, u2;

    while (len-- > 0) {
        u1 = (unsigned char)*str1++;
        u2 = (unsigned char)*str2++;
        if (u1 != u2) {
            return u1 - u2;
        }
        if (u1 == '\0') {
            return 0;
        }
    }

    return 0;
}

int strnlen_terminator(const char* str, int max, char terminator) {
    int i = 0;
    for (i = 0; i < max; i++) {
        if(str[i] == terminator || str[i] == '\0') {
            break;
        }
    }

    return i;
}

int istrncmp(const char* str1, const char* str2, int len) {
    unsigned char u1, u2;

    while (len-- > 0) {
        u1 = (unsigned char)*str1++;
        u2 = (unsigned char)*str2++;
        if (u1 != u2 && tolower(u1) != tolower(u2)) {
            return u1 - u2;
        }
        if (u1 == '\0') {
            return 0;
        }
    }

    return 0;
}

char* strcmp(char* dest, const char* src) {
    char* res = dest;
    while(*src != 0) {
        *dest = *src;
        src++;
        dest++;
    }

    return res;
}

char* strcpy(char* dest, const char* src) {
    char* res = dest;
    while(*src != 0){
        *dest = *src;
        src += 1;
        dest += 1;
    }

    *dest = 0x00;

    return res;
}

bool isdigit(char c) {
    return c >= 48 && c <= 57;
}

// converts an ascii string to an integer
int tonum(char c) {
    return c - 48;
}