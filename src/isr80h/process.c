#include "process.h"
#include "../task/task.h"
#include "../status.h"
#include "../task/process.h"
#include "../string/string.h"
#include "../kernel.h"
#include "../config.h"


void* isr80h_command6_process_load_start(struct interrupt_frame* frame) {
    void* fname_user_ptr = task_get_stack_item(task_current(), 0);
    char fname[BENOS_MAX_PATH];
    int res = copy_string_from_task(task_current(), fname_user_ptr, fname, sizeof(fname));
    if (res < 0) {
        goto out;
    }

    char path[BENOS_MAX_PATH];
    strcpy(path, "0:/");
    strcpy(path+3, fname);

    struct process* process = 0;
    res = process_load_switch(path, &process);
    if (res < 0) {
        goto out;
    }

    task_switch(process->task);
    task_return(&process->task->registers);

out:
    return 0;
}

void* isr80h_command7_invoke_system_command(struct interrupt_frame* frame) {
    struct command_arg* args = task_virt_addr_to_phys(task_current(), task_get_stack_item(task_current(), 0));
    if (!args || strlen(args[0].arg) == 0) {
        return ERROR(-EINVARG);
    }

    struct command_arg* root_command_arg = &args[0];
    const char* program_name = root_command_arg->arg;

    // kinda unoptimized to do this, but whatever
    char path[BENOS_MAX_PATH];
    strcpy(path, "0:/");
    strncpy(path+3, program_name, sizeof(path));

    struct process* process = 0;
    int res = process_load_switch(path, &process);
    if (res < 0) {
        return ERROR(res);
    }

    process_inject_args(process, root_command_arg);
    if (res < 0) {
        return ERROR(res);
    }
    task_switch(process->task);
    task_return(&process->task->registers);

    return 0;
}

void* isr80h_command8_get_program_arguments(struct interrupt_frame* frame) {
    struct process* process = task_current()->process;
    struct process_args* args = task_virt_addr_to_phys(task_current(), task_get_stack_item(task_current(), 0));

    process_get_args(process, &args->argc, &args->argv);
    return 0;
}