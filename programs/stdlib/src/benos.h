#ifndef BENOS_H
#define BENOS_H

#include <stddef.h>
#include <stdbool.h>

void print(const char* fname);
int benos_getkey();

void* benos_malloc(size_t size);
void benos_free(void* ptr);
void benos_putchar(char c);
int benos_getkeyblock();
void benos_terminal_readline(char* out, int max, bool out_while_typing);
void benos_process_load_start(const char* fname);

#endif