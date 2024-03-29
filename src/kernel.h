#ifndef KERNEL_H
#define KERNEL_H

void panic(const char* msg);
void kernel_main();
void print(const char* str);
void ter_writechar(char character, char color);
void kernel_page();
void kernel_registers();

#define VGA_WIDTH 80
#define VGA_HEIGHT 20

#define BENOS_MAX_PATH 108

//macros for error handling
#define ERROR(value) (void*) (value)
#define ERROR_I(value) (int) (value)
#define ISERR(value) ((int) value < 0)

#endif