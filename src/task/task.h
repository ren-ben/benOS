#ifndef TASK_H
#define TASK_H

#include "../config.h"
#include "../memory/paging/paging.h"


struct registers {
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;

    uint32_t ip; // Instruction pointer
    uint32_t cs; // Code segment
    uint32_t flags; // Flags
    uint32_t esp; // Stack pointer
    uint32_t ss; // Stack segment
};

struct task {
    // page dir of the task
    struct paging_4gb_chunk* page_directory;

    // registers of the task when the task isn't running
    struct registers registers;

    // next task in linked list
    struct task* next;

    // previous task in linked list
    struct task* prev;

};

struct task* task_new();
struct task* task_current();
struct task* task_get_next();
int task_free(struct task* task);

#endif