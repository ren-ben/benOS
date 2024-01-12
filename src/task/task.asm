[BITS 32]
section .asm

global restore_general_purpose_registers
global task_return
global user_registers

; void task_return(struct registers* regs) <- regs is getting passed
task_return:
    mov ebp, esp
    ; PUSH DATA SEGMENT (ss is fine)
    ; PUSH STACK ADDRESS
    ; PUSH FLAGS
    ; PUSH CODE SEGMENT
    ; PUSH IP

    mov ebx, [ebp+4]
    ; push data/stack selector
    push dword [ebx+44]
    ; push stack pointer
    push dword [ebx+40]

    ; push flags
    pushf
    pop eax
    or eax, 0x200 ; enables interrupts once we issue an iret
    push eax

    ; push code segment
    push dword [ebx+32]
    ; push the ip to execute
    push dword [ebx+28]
    ; setup some segment regs
    mov ax, [ebx+44]
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push dword [ebp+4]
    call restore_general_purpose_registers
    add esp, 4

    ; leave kernel land and execute user land
    iretd




restore_general_purpose_registers:
    push ebp
    mov ebp, esp
    mov ebx, [ebp+8]
    mov edi, [ebx]
    mov esi, [ebx+4]
    mov ebp, [ebx+8]
    mov edx, [ebx+16]
    mov ecx, [ebx+20]
    mov eax, [ebx+24]
    mov ebx, [ebx+12]
    pop ebp
    ret

; void user_registers()
user_registers:
    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    ret