[BITS 32]

section .asm

global print:function
global benos_getkey:function
global benos_malloc:function
global benos_free:function
global benos_putchar:function
global benos_process_load_start:function
global benos_process_get_args:function
global benos_system:function
global benos_exit:function

; void print(const char* fname)
print:
    push ebp
    mov ebp, esp
    push dword[ebp+8]
    mov eax, 1 ; command print
    int 0x80
    add esp, 4
    pop ebp
    ret

; int getkey()
benos_getkey:
    push ebp
    mov ebp, esp
    mov eax, 2 ; command getkey
    int 0x80
    pop ebp
    ret

; void benos_putchar(char c)
benos_putchar:
    push ebp
    mov ebp, esp
    mov eax, 3 ; command putchar
    push dword[ebp+8] ; variable "c"
    int 0x80
    add esp, 4
    pop ebp
    ret

; void* benos_malloc(size_t size)
benos_malloc:
    push ebp
    mov ebp, esp
    mov eax, 4 ; command malloc
    push dword[ebp+8] ; variable "size"
    int 0x80
    add esp, 4
    pop ebp
    ret

; void benos_free(void* ptr)
benos_free:
    push ebp
    mov ebp, esp
    mov eax, 5 ; command free
    push dword[ebp+8] ; variable "ptr"
    int 0x80
    add esp, 4
    pop ebp
    ret

; void benos_process_load_start(const char* fname)
benos_process_load_start:
    push ebp
    mov ebp, esp
    mov eax, 6 ; command process_load_start (starts a process)
    push dword[ebp+8] ; variable "fname"
    int 0x80
    add esp, 4
    pop ebp
    ret

; int benos_system(struct command_arg* args)
benos_system:
    push ebp
    mov ebp, esp
    mov eax, 7 ; command system (starts a process)
    push dword[ebp+8] ; variable "args"
    int 0x80
    add esp, 4
    pop ebp
    ret

; void benos_process_get_args(struct process_args* args)
benos_process_get_args:
    push ebp
    mov ebp, esp
    mov eax, 8 ; command process_get_args (gets the arguments of the current process)
    push dword[ebp+8] ; variable arguments
    int 0x80
    add esp, 4
    pop ebp
    ret

; void benos_exit()
benos_exit:
    push ebp
    mov ebp, esp
    mov eax, 9 ; command exit
    int 0x80
    add esp, 4
    pop ebp
    ret