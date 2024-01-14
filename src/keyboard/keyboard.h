#ifndef KEYBOARD_H
#define KEYBOARD_H

typedef int (*KEYBOARD_INIT_FUNCTION)();

struct process;

struct keyboard {
    KEYBOARD_INIT_FUNCTION init;
    char name[20];
    struct keyboard *next;
};

void keyboard_init();
void keyboard_backspace(struct process* proc);
void keyboard_push(char c);
char keyboard_pop();
int keyboard_insert(struct keyboard* keyboard);

#endif