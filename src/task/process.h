#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include "../config.h"
#include "task.h"

#define PROCESS_FILETYPE_ELF 0
#define PROCESS_FILETYPE_BINARY 1

typedef unsigned char PROCESS_FILETYPE;
struct process {
    // process id
    uint16_t id;

    char filename[BENOS_MAX_PATH];

    // main process task
    struct task* task;

    // memory (malloc)  allocations of the process
    void* allocations[BENOS_MAX_PROGRAM_ALLOCATIONS];

    PROCESS_FILETYPE filetype;
    union
    {
        // physical pointer to process memory
        void* ptr;
        struct elf_file* elf;
    };

    // the physical pointer to the stack
    void* stack;

    // size of data pointed to by ptr
    uint32_t size;

    struct keyboard_buffer {
        char buffer[BENOS_KEYBOARD_BUFFER_SIZE];
        int tail;
        int head;
    } keyboard;
};

int process_switch(struct process* proc);
int process_load_switch(const char* fname, struct process** process);
int process_load_for_slot(const char* fname, struct process** process, int process_slot);
int process_load(const char* fname, struct process** process);
struct process* process_current();
struct process* process_get(int process_id);
void* process_malloc(struct process* process, size_t size);

#endif