#include "kernel.h"
#include <stdint.h>
#include <stddef.h>
#include "idt/idt.h"
#include "io/io.h"
#include "string/string.h"
#include "task/task.h"
#include "task/process.h"
#include "memory/heap/kheap.h"
#include "memory/paging/paging.h"
#include "isr80h/isr80h.h"
#include "disk/disk.h"
#include "fs/pparser.h"
#include "disk/streamer.h"
#include "fs/file.h"
#include "gdt/gdt.h"
#include "config.h"
#include "memory/memory.h"
#include "task/tss.h"
#include "status.h"
#include "keyboard/keyboard.h"

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

void terminal_backspace() {
    if (ter_row == 0 && ter_column == 0) {
        return;
    }
    if (ter_column == 0) {
        ter_column = VGA_WIDTH;
        ter_row--;
    } 
    
    ter_column--;
    ter_writechar(' ', 15);
    ter_column--;
}

//writes a character to the screen
void ter_writechar(char character, char color) {
    if (character == '\n') {
        ter_column = 0;
        ter_row++;
        return;
    }

    if (character == 0x08) {
        terminal_backspace();
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

//kernel panic inducer;
void panic(const char* msg) {
    print(msg);
    for (;;) ;
}

void kernel_page() {
    kernel_registers();
    paging_switch(kernel_chunk);
}

struct tss tss;

struct gdt gdt_real[BENOS_TOTAL_GDT_SEGMENTS];
struct gdt_structured gdt_structured[BENOS_TOTAL_GDT_SEGMENTS] = {
    {.base = 0x00, .limit = 0x00, .type = 0x00},                    // null segment
    {.base = 0x00, .limit = 0xFFFFFFFF, .type = 0x9A},              // code segment
    {.base = 0x00, .limit = 0xFFFFFFFF, .type = 0x92},              // data segment
    {.base = 0x00, .limit = 0xFFFFFFFF, .type = 0xF8},              // user code segment
    {.base = 0x00, .limit = 0xFFFFFFFF, .type = 0xF2},              // user data segment
    {.base = (uint32_t)&tss, .limit = sizeof(tss), .type = 0xE9},   // tss segment
};

void kernel_main() {

    ter_init();

    memset(gdt_real, 0x00, sizeof(gdt_real));
    gdt_structured_to_gdt(gdt_real, gdt_structured, BENOS_TOTAL_GDT_SEGMENTS);

    // load the GDT
    gdt_load(gdt_real, sizeof(gdt_real));

    // initialize the kernel heap
    kheap_init();

    // initialize the filesystem
    fs_init();

    // search and initialize the disk
    disk_search_and_init();

    // initialize the IDT
    idt_init();

    // setup the tss
    memset(&tss, 0x00, sizeof(tss));
    tss.esp0 = 0x600000;
    tss.ss0 = KERNEL_DATA_SELECTOR;
    
    // load the tss
    tss_load(0x28);

    // setup paging
    kernel_chunk = paging_new_4gb(PAGING_IS_WRITABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);
    paging_4g_chunk_get_dir(kernel_chunk);
    paging_switch(kernel_chunk);

    /*char* ptr = kzalloc(4096);
    paging_set(paging_4g_chunk_get_dir(kernel_chunk), (void*)0x1000, (uint32_t)ptr | PAGING_IS_WRITABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);*/

    // enable paging
    enable_paging();

    // initialize the isr80h
    isr80h_register_commands();

    // initialize the keyboard
    keyboard_init();


    /*
    char* ptr2 = (char*) 0x1000;
    ptr2[0] = 'a';
    ptr2[1] = 'b';
    print(ptr2);
    print(ptr);

    -> ptr and ptr2 are the same address
    */

   struct process* process = 0;
   int res = process_load_switch("0:/blank.elf", &process);
   if (res != BENOS_ALL_OK) {
         panic("Failed to load process!");
   }

   task_run_first_ever_task();

    for(;;) {}
}