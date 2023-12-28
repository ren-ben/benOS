ENTRY(_start)
OUTPUT_FORMAT(binary)
SECTIONS
{
    . = 1M; /* Kernel starts at 1 MB in memory */
    .text :
    {
        *(.text)
    }

    .rodata :
    {
        *(.rodata*)
    }

    .data :
    {
        *(.data)
    }

    .bss :
    {
        *(COMMON)
        *(.bss)
    }
}