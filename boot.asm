ORG 0

BITS 16

_start:
    jmp short start
    nop

times 33 db 0 ; bc BIOS parameter block overrides stuff (33 bytes in size)

start:
    jmp 0x7C0 : step2

step2:
    cli ; clear interrupts

    mov ax, 0x7c0
    mov ds, ax
    mov es, ax

    mov ax, 0x00
    mov ss, ax
    mov sp, 0x7C00

    sti ; enable interrupts

    ; http://www.ctyme.com/intr/rb-0607.htm
    mov ah, 2 ; read sector function
    mov al, 1 ; one sector to read
    mov ch, 0 ; cylinder low eight bits
    mov cl, 2 ; sector number
    mov dh, 0 ; head number
    mov bx, buffer ; buffer address
    int 0x13 ; BIOS interrupt
    jc error ; jump if carry flag is set



    mov si, buffer
    call print

    jmp $

error:
    mov si, error_msg
    call print
    jmp $

print:
    mov bx, 0
.loop:
    lodsb
    cmp al, 0
    je .done
    call printchar
    jmp .loop
.done:
    ret

printchar:
    mov ah, 0eh
    int 0x10
    ret

error_msg db "Failed to load sector", 0

times 510-($-$$) db 0
dw 0xAA55

buffer: