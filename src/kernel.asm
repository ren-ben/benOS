[BITS 32]
global _start ; tell linker entry point
extern kernel_main ; tell linker where to find kernel_start
CODE_SEG equ 0x08
DATA_SEG equ 0x10

_start:
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

    ; remap master PIC
    mov al, 00010001b ; ICW1
    out 0x20, al ; write to master PIC command port

    mov al, 0x20 ; int 0x20 is where master isr starts
    out 0x21, al ; write to master PIC data port

    mov al, 00000001b
    out 0x21, al

    call kernel_main
    jmp $

times 512-($-$$) db 0 ; for solving allignment isues
