ORG 0x7c00
BITS 16

CODE_SEG equ gdt_code - gdt_start ; sets offset of code segment (0x8)
DATA_SEG equ gdt_data - gdt_start ; sets offset of data segment (0x10)

_start:
    jmp short start
    nop

times 33 db 0 ; bc BIOS parameter block overrides stuff (33 bytes in size)

start:
    jmp 0:step2

step2:
    cli ; clear interrupts
    mov ax, 0x00
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00
    sti ; enable interrupts

.load_protected:
    cli
    lgdt[gdt_descriptor] ; load GDT
    mov eax, cr0
    or eax, 0x1 ; set protected mode bit
    mov cr0, eax
    jmp CODE_SEG:load32 ; jump to 32-bit code


; GDT

gdt_start:
gdt_null: ; 64 bits of 0's (null descriptor)
    dd 0x0
    dd 0x0

; offset 0x8
gdt_code: ; 64 bits of code descriptor (CS points to this)
    dw 0xffff ; segment limit first 0-15 bits
    dw 0 ; base address first 0-15 bits
    db 0 ; base address next 16-23 bits
    db 0x9a ; access flags (bitmask)
    db 11001111b ; flags (bitmask)
    db 0 ; base address last 24-31 bits

; offset 0x10
gdt_data: ; 64 bits of data descriptor (DS, SS, ES, FS, GS point to this)
    dw 0xffff ; segment limit first 0-15 bits
    dw 0 ; base address first 0-15 bits
    db 0 ; base address next 16-23 bits
    db 0x92 ; access flags (bitmask)
    db 11001111b ; flags (bitmask)
    db 0 ; base address last 24-31 bits

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start-1 ; size of GDT
    dd gdt_start ; start of GDT

[BITS 32]

load32:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov ebp, 0x00200000
    mov esp, ebp

    ; enable A20 line
    in al, 0x92 ; read from port 0x92 (bus)
    or al, 2 ; set bit 1
    out 0x92, al ; write to port 0x92 (bus)

    jmp $

times 510-($-$$) db 0

dw 0xAA55