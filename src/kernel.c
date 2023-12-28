#include "kernel.h"
#include <stdint.h>
#include <stddef.h>

//a pointer to vmemory
uint16_t* video_memory = 0;

uint16_t ter_row;
uint16_t ter_column;

//outputs a character to the screen
uint16_t ter_make_character(char character, char color) {
    return (color << 8) | character;
}

//puts a character on the screen
void ter_putchar(int x, int y, char character, char color) {
    video_memory[y * VGA_WIDTH + x] = ter_make_character(character, color);
}

//writes a character to the screen
void ter_writechar(char character, char color) {
    if (character == '\n') {
        ter_column = 0;
        ter_row++;
        return;
    }
    ter_putchar(ter_column, ter_row, character, color);
    ter_column++;
    if (ter_column >= VGA_WIDTH) {
        ter_column = 0;
        ter_row++;
    }
}

// clears the screen
void ter_init() {
    video_memory = (uint16_t*) 0xB8000;

    ter_column = 0;
    ter_row = 0;

    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            ter_putchar(x, y, ' ', 0);
        }
    }
}

// gets the length of a string
size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len])
        len++;
    return len;
}

// prints a string to the screen
void print(const char* str) {
    for (size_t i = 0; i < strlen(str); i++) {
        ter_writechar(str[i], 0x0F);
    }
}


void kernel_main() {
    ter_init();
    print("Hello, World!");
}