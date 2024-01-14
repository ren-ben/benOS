#include "process.h"
#include "../config.h"
#include "../status.h"
#include "../memory/memory.h"
#include "../memory/heap/kheap.h"
#include "../fs/file.h"
#include "../string/string.h"
#include "../kernel.h"
#include "../memory/paging/paging.h"
#include "../loader/formats/elfloader.h"


// current running process
static struct process* curr_process = 0;

static struct process* processes[BENOS_MAX_PROCESSES] = {};

static void process_init(struct process* process) {
    memset(process, 0, sizeof(struct process));
}

struct process* process_current() {
    return curr_process;
}


struct process* process_get(int process_id) {
    if (process_id < 0 || process_id >= BENOS_MAX_PROCESSES) {
        return NULL;
    }

    return processes[process_id];
}

int process_switch(struct process* proc) {
    curr_process = proc;
    return 0;
}

static int process_load_binary(const char* fname, struct process* process) {
    int res = 0;

    int fd = fopen(fname, "r");
    if (!fd) {
        res = -EIO;
        goto out;
    }

    struct file_stat stat;
    res = fstat(fd, &stat);

    if (res != BENOS_ALL_OK) {
        goto out;
    }

    void* program_data_ptr = kzalloc(stat.size);
    if (!program_data_ptr) {
        res = -ENOMEM;
        goto out;
    }

    if (fread(program_data_ptr, stat.size, 1, fd) != 1) {
        res = -EIO;
        goto out;
    }

    process->filetype = PROCESS_FILETYPE_BINARY;
    process->ptr = program_data_ptr;
    process->size = stat.size;

out:
    fclose(fd);
    return res;
}

static int process_load_elf(const char* fname, struct process* process) {
    int res = 0;
    struct elf_file* elf_file = 0;
    res = elf_load(fname, &elf_file);

    if (ISERR(res)) {
        goto out;
    }

    process->filetype = PROCESS_FILETYPE_ELF;
    process->elf = elf_file;
out:
    return res;
}

static int process_load_data(const char* fname, struct process* process) {
    int res = 0;
    res = process_load_elf(fname, process);
    if (res == -EINFORMAT) {
        res = process_load_binary(fname, process);
    }
    return res;
}

int process_map_binary(struct process* process) {
    int res = 0;
    paging_map_to(process->task->page_directory, (void*) BENOS_PROGRAM_VIRTUAL_ADDRESS, process->ptr, paging_align_address(process->ptr + process->size), PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL | PAGING_IS_WRITABLE);
    return res; 
}

static int process_map_elf(struct process* process) {
    int res = 0;
    struct elf_file* elf_file = process->elf;
    struct elf_header* header = elf_header(elf_file);
    struct elf32_phdr* phdrs = elf_pheader(header);
    for (int i = 0; i < header->e_phnum; i++) {
        struct elf32_phdr* phdr = &phdrs[i];
        void* phdr_phys_address = elf_phdr_phys_address(elf_file, phdr);
        int flags = PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL;
        if (phdr->p_flags & PF_W) {
            flags |= PAGING_IS_WRITABLE;
        }
        res = paging_map_to(process->task->page_directory, paging_align_to_lower_page((void*)phdr->p_vaddr), paging_align_to_lower_page(phdr_phys_address), paging_align_address(phdr_phys_address+phdr->p_filesz), flags);
        if (ISERR(res)) {
            break;
        }
    }
    return res;
}

int process_map_memory(struct process* process) {
    int res = 0;

    switch(process->filetype) {
        case PROCESS_FILETYPE_ELF:
            res = process_map_elf(process);
            break;
        case PROCESS_FILETYPE_BINARY:
            res = process_map_binary(process);
            break;
        default:
            panic("Unknown process filetype\n");
    }
    res = process_map_binary(process);
    if (res < 0) {
        goto out;
    }

    paging_map_to(process->task->page_directory, (void*)BENOS_PROGRAM_VIRTUAL_STACK_ADDRESS_END, process->stack, paging_align_address(process->stack + BENOS_USER_PROGRAM_STACK_SIZE), PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL | PAGING_IS_WRITABLE);

out:
    return res;
}

int process_get_free_slot() {
    for (int i = 0; i < BENOS_MAX_PROCESSES; i++) {
        if (processes[i] == 0) {
            return i;
        }
    }

    return -EISTKN;

}

int process_load(const char* fname, struct process** process) {
    int res = 0;
    int process_slot = process_get_free_slot();


    if (process_slot < 0) {
        res = -EISTKN;
        goto out;
    }

    res = process_load_for_slot(fname, process, process_slot);

out:
    return res;
}

int process_load_switch(const char* fname, struct process** process) {
    int res = process_load(fname, process);
    if (res == 0) {
        process_switch(*process);
    }

    return res;
}

int process_load_for_slot(const char* fname, struct process** process, int process_slot) {
    int res = 0;
    struct task* task = 0;
    struct process* _process;
    void* program_stack_ptr = 0;

    if (process_get(process_slot) != 0) {
        res = -EISTKN;
        goto out;
    }

    _process = kzalloc(sizeof(struct process));
    if (!_process) {
        res = -ENOMEM;
        goto out;
    }

    process_init(_process);
    res = process_load_data(fname, _process);
    if (res < 0) {
        goto out;
    }

    program_stack_ptr = kzalloc(BENOS_USER_PROGRAM_STACK_SIZE);

    if (!program_stack_ptr) {
        res = -ENOMEM;
        goto out;
    }

    strncpy(_process->filename, fname, sizeof(_process->filename));
    _process->stack = program_stack_ptr;
    _process->id = process_slot;

    // create a new task
    task = task_new(_process);

    if (ERROR_I(task) == 0) {
        res = ERROR_I(task);
        goto out;
    }

    _process->task = task;

    res = process_map_memory(_process);
    if (res < 0) {
        goto out;
    }

    *process = _process;

    // add process to array
    process[process_slot] = _process;


out:
    if (ISERR(res)) {
        if (_process && _process->task) {
            task_free(_process->task);
        }

        kfree(_process);
    }
    return res;
}