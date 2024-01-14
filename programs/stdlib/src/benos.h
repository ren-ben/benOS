#ifndef BENOS_H
#define BENOS_H

#include <stddef.h>

void print(const char* fname);
int getkey();

void* benos_malloc(size_t size);
void benos_free(void* ptr);
void benos_putchar(char c);

#endif