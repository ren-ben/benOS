#include "stdio.h"
#include "benos.h"
#include "stdlib.h"
#include <stdarg.h>

int putchar(int c) {
    benos_putchar((char)c);
    return 0;
}

int printf(const char* fmt, ...) {
    va_list args;
    const char* p;
    char *sval;
    int ival;

    va_start(args, fmt);
    for (p = fmt; *p; p++) {
        if (*p != '%') {
            putchar(*p);
            continue;
        }

        switch (*++p) {
            case 'i':
                ival = va_arg(args, int);
                print(itoa(ival));
                break;
            case 's':
                sval = va_arg(args, char*);
                print(sval);
                break;
            default:
                putchar(*p);
                break;
        }
    }

    va_end(args);
    return 0;
}