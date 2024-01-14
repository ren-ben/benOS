#ifndef ELFLOADER_H
#define ELFLOADER_H

#include <stdint.h>
#include <stddef.h>

#include "elf.h"
#include "../../config.h"

struct elf_file {
    char fname[BENOS_MAX_PATH];

    int in_mem_size;

    //phys mem address that this efl file is loaded at
    void* elf_mem;

    //virt base address of this binary
    void* virt_base_address;

    //virt end address of this binary
    void* virt_end_address;

    //phys base address of this binary
    void* phys_base_address;

    //phys end address of this binary
    void* phys_end_address;
};

int elf_load(const char* fname, struct elf_file** file_out);
void elf_close(struct elf_file* file);
void* elf_virtual_base(struct elf_file* file);
void* elf_virtual_end(struct elf_file* file);
void* elf_phys_base(struct elf_file* file);
void* elf_phys_end(struct elf_file* file);

struct elf_header* elf_header(struct elf_file* file);
struct elf32_shdr* elf_sheader(struct elf_header* header);
void* elf_memory(struct elf_file* file);
struct elf32_phdr* elf_pheader(struct elf_header* header);
struct elf32_phdr* elf_program_header(struct elf_header* header, int index);
struct elf32_shdr* elf_section(struct elf_header* header, int index);
void* elf_phdr_phys_address(struct elf_file* file, struct elf32_phdr* phdr);

#endif