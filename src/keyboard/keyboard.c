#include "keyboard.h"
#include "../status.h"
#include "../kernel.h"
#include "../task/process.h"
#include "../task/task.h"
#include "classic.h"

static struct keyboard* keyboard_list_head = 0;
static struct keyboard* keyboard_list_tail = 0;

void keyboard_init() {
    keyboard_insert(classic_init());
}

int keyboard_insert(struct keyboard* keyboard) {
    int res = 0;
    if (keyboard->init == 0) {
        res = -EINVARG;
        goto out;
    }

    if (keyboard_list_tail) {
        keyboard_list_tail->next = keyboard;
        keyboard_list_tail = keyboard;
    } else {
        keyboard_list_head = keyboard;
        keyboard_list_tail = keyboard;
    }

    res = keyboard->init();

out:
    return res;
}

static int keyboard_get_tail_index(struct process* proc) {
    return proc->keyboard.tail % sizeof(proc->keyboard.buffer);
}

void keyboard_backspace(struct process* proc) {
    proc->keyboard.tail--;
    int real_i = keyboard_get_tail_index(proc);
    proc->keyboard.buffer[real_i] = 0x00;
}

void keyboard_push(char c) {
    struct process* proc = process_current();
    if(!proc) {
        return;
    }

    if (c == 0) {
        return;
    }

    int real_i = keyboard_get_tail_index(proc);
    proc->keyboard.buffer[real_i] = c;
    proc->keyboard.tail++;
}

char keyboard_pop() {
    if (!task_current()) {
        return 0;
    }

    struct process* proc = task_current()->process;
    int real_i = proc->keyboard.head % sizeof(proc->keyboard.buffer);
    char c = proc->keyboard.buffer[real_i];
    if (c == 0x00) {
        return 0; // nothing to pop, return 0
    }

    proc->keyboard.buffer[real_i] = 0x00;
    proc->keyboard.head++;
    return c;
}