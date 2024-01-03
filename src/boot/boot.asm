ORG 0x7c00
BITS 16

CODE_SEG equ gdt_code - gdt_start ; sets offset of code segment (0x8)
DATA_SEG equ gdt_data - gdt_start ; sets offset of data segment (0x10)


jmp short start
nop

; FAT16 Header
OEMIdentifier        db "BENOS   "
BytesPerSector       dw 0x200
SectorsPerCluster    db 0x80
ReservedSectors      dw 200
FATCopies            db 0x02
RootDirectoryEntries dw 0x40
NumSectors           dw 0x00
MediaType            db 0xF8
SectorsPerFAT        dw 0x100
SectorsPerTrack      dw 0x20
NumOfHeads           dw 0x40
HiddenSectors        dd 0x00
SectorsBig           dd 0x773594

; Extended BPB (Dos 4.0)
DriveNumber          db 0x80
WinNTBit             db 0x00
Signature            db 0x29
VolumeID             dd 0xD105
; exactly 11 bytes
VolumeIDString       db "BENOS BOOT "
SystemIDString       db "FAT16   "

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

[BITS 32] ; 32-bit code

; writing our loading driver 
load32:
    mov eax, 1 ; starting sector
    mov ecx, 100 ; number of sectors to read
    mov edi, 0x0100000 ; destination address to load sectors into
    call ata_lba_read
    jmp CODE_SEG:0x0100000 ; jump to loaded sectors

ata_lba_read:
    ; send highest 8 bits to hd controller
    mov ebx, eax ; backup the LBA
    shr eax, 24 ; shift eax right 24 bits (highest 8 bits of LBA are left)
    or eax, 0xe0 ; set highest 3 bits to 1 (LBA mode) -> select master drive
    mov dx, 0x1f6 ; port hd controller
    out dx, al ; send highest 8 bits of LBA to port hd controller (bus)

    ; send total sectors to read
    mov eax, ecx ; backup total sectors to read
    mov dx, 0x1f2 ; port 
    out dx, al ; send total sectors to read to port hd controller (bus)

    ; send LBA to hd controller
    mov eax, ebx ; restore LBA
    mov dx, 0x1f3 ; port 
    out dx, al ; send LBA to port hd controller (bus)

    ; send next 8 bits of LBA to hd controller
    mov dx, 0x1f4 ; port 
    mov eax, ebx ; restore LBA
    shr eax, 8 ; shift eax right 8 bits (next 8 bits of LBA are left)
    out dx, al ; send next 8 bits of LBA to port hd controller (bus)

    ; send upper 16 bits of LBA to hd controller
    mov dx, 0x1f5 ; port
    mov eax, ebx ; restore LBA
    shr eax, 16 ; shift eax right 16 bits (upper 16 bits of LBA are left)
    out dx, al ; send upper 16 bits of LBA to port hd controller (bus)

    mov dx, 0x1f7 ; port
    mov al, 0x20 ; read with retry
    out dx, al ; send read with retry command to port hd controller (bus)

    ; read all sectors into memory

.next_sector:
    push ecx ; backup ecx

.try_again:
    mov dx, 0x1f7 ; port
    in al, dx ; read status from port hd controller (bus)
    test al, 8 ; test if error bit is set
    jz .try_again ; if not set, try again

; read 256 words at a time
    mov cx, 256 ; read 256 words at a time
    mov dx, 0x1f0 ; port
    rep insw ; read 256 words from port hd controller (bus) into memory
    ; do the insw instruction 256 times (word = 2bytes, 2*256=512 bytes=1 sector)

    pop ecx ; restore ecx
    loop .next_sector ; loop until all sectors are read

    ret


times 510-($-$$) db 0

dw 0xAA55