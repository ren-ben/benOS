#ifndef TASK_H
#define TASK_H

#include "../config.h"
#include "../memory/paging/paging.h"


struct interrupt_frame;

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

struct process;

struct task {
    // page dir of the task
    struct paging_4gb_chunk* page_directory;

    // registers of the task when the task isn't running
    struct registers registers;
    
    // process of the task
    struct process* process;

    // next task in linked list
    struct task* next;

    // previous task in linked list
    struct task* prev;

};

struct task* task_new(struct process* process);
struct task* task_current();
struct task* task_get_next();
int task_free(struct task* task);

int task_switch(struct task* task);
int task_page();
int task_page_task(struct task* task);

void task_run_first_ever_task();
void task_return(struct registers* regs);
void restore_general_purpose_registers(struct registers* regs);
void user_registers();

void task_current_save_state(struct interrupt_frame* frame);
int copy_string_from_task(struct task* task, void* virt, void* phys, int max);
void* task_get_stack_item(struct task* task, int index);
void* task_virt_addr_to_phys(struct task* task, void* virt);

#endif