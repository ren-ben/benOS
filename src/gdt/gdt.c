#include "gdt.h"
#include "kernel.h"

void encoded_gdt_entry(uint8_t* target, struct gdt_structured source) {
    if (source.limit > 65536 && (source.limit & 0xFFF) != 0xFFF) {
        panic("Kernel Panic: encodedGdtEntry: Invalid argument\n");
    }

    target[6] = 0x40;

    if (source.limit > 65536) {
        source.limit = source.limit >> 12;
        target[6] = 0xC0;
    }

    //encodes the limit
    target[0] = source.limit & 0xFF;
    target[1] = (source.limit >> 8) & 0xFF;
    target[6] |= (source.limit >> 16) & 0xF;

    //encode the base
    target[2] = source.base & 0xFF;
    target[3] = (source.base >> 8) & 0xFF;
    target[4] = (source.base >> 16) & 0xFF;
    target[7] = (source.base >> 24) & 0xFF;

    //encode type
    target[5] = source.type;
}

void gdt_structured_to_gdt(struct gdt* gdt, struct gdt_structured* gdt_structured, int total_entries) {
    for (int i = 0; i < total_entries; i++){
        encoded_gdt_entry((uint8_t*)&gdt[i], gdt_structured[i]);
    }
    
}