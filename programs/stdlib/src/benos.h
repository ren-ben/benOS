#ifndef BENOS_H
#define BENOS_H

#include <stddef.h>
#include <stdbool.h>

struct command_arg {
    char arg[512];
    struct command_arg* next;
};

struct process_args {
    int argc;
    char** argv;
};

void print(const char* fname);
int benos_getkey();

void* benos_malloc(size_t size);
void benos_free(void* ptr);
void benos_putchar(char c);
int benos_getkeyblock();
void benos_terminal_readline(char* out, int max, bool out_while_typing);
void benos_process_load_start(const char* fname);
struct command_arg* benos_parse_command(const char* command, int max);
void benos_process_get_args(struct process_args* args);

#endif