## Text Mode

- allows you to write ascii to video memory
- starting at 0xB8000 for colored displays
- monochrome -> 0xB0000
- each ascii written to this memory has its pixel equivalent outputted to the monitor

- each character takes up 2 bytes
  - byte 0 = ascii character
  - byte 1 = color code
  - Example:
    - 0xb8000 = "A"
    - 0xb8001 = 0x00
- when defining them, you can define both the color and the char at once, but you gotta be careful cuz of little endianess (color first, then char)
  > example: for char 0x41 and color 0x03 we would say `video mem[0] = 0x0341;`

## Interrupt Descriptor Table

- describes how interrupts are invoked in protected mode
- think of an array of interrupt descriptors

## programmable interrupt controller

- allows hardware to interrupt the processor
