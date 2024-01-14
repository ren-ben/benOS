[BITS 32]

section .asm

global print:function
global getkey:function
global benos_malloc:function
global benos_free:function

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
getkey
    push ebp
    mov ebp, esp
    mov eax, 2 ; command getkey
    int 0x80
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