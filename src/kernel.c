#include "kernel.h"
#include <stdint.h>
#include <stddef.h>
#include "idt/idt.h"
#include "io/io.h"
#include "string/string.h"
#include "memory/heap/kheap.h"
#include "memory/paging/paging.h"
#include "disk/disk.h"
#include "fs/pparser.h"
#include "disk/streamer.h"
#include "fs/file.h"

//a pointer to vmemory
uint16_t* video_memory = 0;

uint16_t ter_row = 0;
uint16_t ter_column = 0;

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

// prints a string to the screen
void print(const char* str) {
    for (size_t i = 0; i < strlen(str); i++) {
        ter_writechar(str[i], 0x0F);
    }
}

static struct paging_4gb_chunk* kernel_chunk = 0;

void kernel_main() {

    ter_init();

    // initialize the kernel heap
    kheap_init();

    // initialize the filesystem
    fs_init();

    // search and initialize the disk
    disk_search_and_init();

    // initialize the IDT
    idt_init();

    // setup paging
    kernel_chunk = paging_new_4gb(PAGING_IS_WRITABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);
    paging_4g_chunk_get_dir(kernel_chunk);
    paging_switch(kernel_chunk->directory_entry);

    /*char* ptr = kzalloc(4096);
    paging_set(paging_4g_chunk_get_dir(kernel_chunk), (void*)0x1000, (uint32_t)ptr | PAGING_IS_WRITABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);*/

    // enable paging
    enable_paging();

    /*
    char* ptr2 = (char*) 0x1000;
    ptr2[0] = 'a';
    ptr2[1] = 'b';
    print(ptr2);
    print(ptr);

    -> ptr and ptr2 are the same address
    */

    // enable interrupts
    enable_interrupts();

    int fd = fopen("0:/hello.txt", "r");
    if (fd) {
        print("\nwe opened hello.txt\n");
        char buff[14];
        fread(buff, 13, 1, fd);
        buff[13] = 0x00;
        print(buff);
    }
    while(1) {}
}