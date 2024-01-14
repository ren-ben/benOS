#include "task.h"
#include "../kernel.h"
#include "../status.h"
#include "../memory/paging/paging.h"
#include "../memory/heap/kheap.h"
#include "../memory/memory.h"
#include "../task/process.h"
#include "../idt/idt.h"
#include "../memory/paging/paging.h"
#include "../string/string.h"
#include "../loader/formats/elfloader.h"

// current running task
struct task* current_task = 0;

// task linked list
struct task* task_tail = 0;
struct task* task_head = 0;

int task_init(struct task* task, struct process* process);

struct task* task_current() {
    return current_task;
}

struct task* task_new(struct process* process) {
    int res = 0;
    struct task* task = kzalloc(sizeof(struct task));
    if (!task) {
        res = -ENOMEM;
        goto out;
    }

    res = task_init(task, process);
    if (res != BENOS_ALL_OK) {
        goto out;
    }

    if (task_head == 0) {
        task_head = task;
        task_tail = task;
        current_task = task;
        goto out;
    }

    task_tail->next = task;
    task->prev = task_tail;
    task_tail = task;

out:
    if (ISERR(res)) {
        task_free(task);
        return ERROR(res);
    }
    return task;
}

struct task* task_get_next() {
    if (!current_task->next) {
        return task_head;
    }

    return current_task->next;
}

static void task_list_remove(struct task* task) {

    if (task->prev) {
        task->prev->next = task->next;
    }

    if (task == task_head) {
        task_head = task->next;
    }

    if (task == task_tail) {
        task_tail = task->prev;
    }

    if (task == current_task) {
        current_task = task_get_next();
    }
}

int task_free(struct task* task) {
    paging_free_4gb(task->page_directory);
    task_list_remove(task);

    // finally free the task data
    kfree(task);
    return 0;
}

int task_switch(struct task* task) {
    current_task = task;
    paging_switch(task->page_directory);
    return 0;
}

// saves the state of the current task
void task_save_state(struct task* task, struct interrupt_frame* frame) {
    task->registers.ip = frame->ip;
    task->registers.cs = frame->cs;
    task->registers.flags = frame->flags;
    task->registers.esp = frame->esp;
    task->registers.ss = frame->ss;
    task->registers.eax = frame->eax;
    task->registers.ebp = frame->ebp;
    task->registers.ebx = frame->ebx;
    task->registers.ecx = frame->ecx;
    task->registers.edi = frame->edi;
    task->registers.edx = frame->edx;
    task->registers.esi = frame->esi;

}

int copy_string_from_task(struct task* task, void* virt, void* phys, int max) {
    if (max >= PAGING_PAGE_SIZE) {
        return -EINVARG;
    }

    int res = 0;
    char* tmp = kzalloc(max);
    if (!tmp) {
        res = -ENOMEM;
        goto out;
    }

    uint32_t* task_dir = task->page_directory->directory_entry;
    uint32_t old_entry = paging_get(task_dir, tmp);
    paging_map(task->page_directory, tmp, tmp, PAGING_IS_WRITABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);
    paging_switch(task->page_directory);
    strncpy(tmp, virt, max);
    kernel_page();

    res = paging_set(task_dir, tmp, old_entry);
    if (res < 0) {
        res = -EIO;
        goto out_free;
    }

    strncpy(phys, tmp, max);

out_free:
    kfree(tmp);

out:
    return res;

}

void task_current_save_state(struct interrupt_frame* frame) {
    if (!task_current()) {
        panic("task_current_save_state(): No current task exists!\n");
    }

    struct task* task = task_current();
    task_save_state(task, frame);
}

int task_page() {
    user_registers();
    task_switch(current_task);
    return 0;
}

int task_page_task(struct task* task) {
    user_registers();
    paging_switch(task->page_directory);
    return 0;
}

void task_run_first_ever_task() {
    if (!current_task) {
        panic("task_run_first_ever_task(): No current task exists!\n");
    }

    task_switch(task_head);
    task_return(&task_head->registers);
}

int task_init(struct task* task, struct process* process) {
    memset(task, 0, sizeof(struct task));
    // map entire 4gb address space to itself
    task->page_directory = paging_new_4gb(PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);
    if (!task->page_directory) {
        return -EIO;
    }

    

    // ip -> when the task was executing before an interrupt (we're in charge of setting this)
    task->registers.ip = BENOS_PROGRAM_VIRTUAL_ADDRESS;

    if (process->filetype == PROCESS_FILETYPE_ELF) {
        task->registers.ip = elf_header(process->elf)->e_entry;
    }

    task->registers.ss = USER_DATA_SEGMENT;
    task->registers.cs = USER_CODE_SEGMENT;
    task->registers.esp = BENOS_PROGRAM_VIRTUAL_STACK_ADDRESS_START;

    task->process = process;

    return 0;
}

void* task_get_stack_item(struct task* task, int index) {
    void* result = 0;

    uint32_t* sp_ptr = (uint32_t*)task->registers.esp;

    // switch to the task's page directory
    task_page_task(task);

    result = (void*)sp_ptr[index];

    // switch to the kernel page directory
    kernel_page();

    return result;
}

void* task_virt_addr_to_phys(struct task* task, void* virt) {
    return paging_get_phys_addr(task->page_directory->directory_entry, virt);
}