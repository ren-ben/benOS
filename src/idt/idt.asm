section .asm

extern int21h_handler
extern no_interrupt_handler
extern isr80h_handler
global int21h
global no_interrupt
global idt_load
global enable_interrupts
global disable_interrupts
global isr80h_wrapper

disable_interrupts:
    cli
    ret

enable_interrupts:
    sti
    ret

idt_load:
    push ebp
    mov ebp, esp

    mov ebx, [ebp+8]
    lidt [ebx]

    pop ebp
    ret

int21h:
    cli
    pushad ; push all registers
    call int21h_handler
    popad ; pop all registers
    sti
    iret

no_interrupt:
    cli
    pushad ; push all registers
    call no_interrupt_handler
    popad ; pop all registers
    sti
    iret


isr80h_wrapper:
    cli
    ; INTERRUPT FRAME START
    ; ALREADY PUSHED BY CPU UPON ENTRY TO THIS INT
    ; ip, cs, flags, sp, ss (32bit)
    pushad ; push all registers to stack

    ; INT FRAME END

    ; push stack pointer so that we point to int frame#
    push esp

    ; eax holds the command (push to the stack for isr80h_handler)
    push eax
    call isr80h_handler
    mov dword[tmp_res], eax
    add esp, 8 ; remove the pushed esp and eax

    ; restore gpr's for user land
    popad
    mov eax, [tmp_res]
    iretd

section .data
; stores return results from isr80h_handler
tmp_res dd 0