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
  > example: for char 0x41 and color 0x03 you would say `video mem[0] = 0x0341;`

## Interrupt Descriptor Table

- describes how interrupts are invoked in protected mode
- think of an array of interrupt descriptors

## Programmable Interrupt Controller

- allows hardware to interrupt the processor
- master handles irq 0-7
- slave handles irq 8-15

## Heap

- the heap is a giant memory region that can be shared in a controlled manner.
- heap implementations -> essentially system memory managers
- malloc in c returns a mem address that you can write to.
- free in c accepts the mem address that you want to free.

> there are memory limits though. In 32 bits, you can address up to 4.29GB of ram.

- the heap can be pointed at an address unused by hardware that is also big enough for us to use
- the heap will work fine with at least 100MB of heap mem
- heap implementation is needed
- heap implementation will be responsible for managing this giant chunk of memeory that is called the heap.

### simplest heap

- start with a start address and call is curr addr point it somewhere free i.e (0x01000000)
- any call to malloc gets the current address stores it in a temporary variable called "tmp"
- now the current address is incr. by the size provided to malloc
- "tmp" that contains the allocated address is returned
- curr_addr now contains the next address for malloc to return when malloc is called again.
  > unfortunately, you can't free memory. So I'll use something else

### my heap implementation

- consists of a giant table which describes a giant piece of free memory in the system. This table will describe which memory is taken, which memory is free etc. It's called the "entry table"
- has another pointer to a giant piece of memory called the "data pool". It will be 100MB in size
- The heap impl. will be block based. Each address returned from malloc will be aligned to 4096 and will at least be 4096 in size.
- If you request to have 50 bytes, 4096 bytes of memory will be returned.

#### entry table

- we want a 100mb heap then the math is

  - 100mb / 4096 = 25600 bytes in our entry table

- entry structure:
  - upper 4 bits are flags -> has_n, is_first, 0, 0
  - lower 4 bits are entry table -> et_2, et_1, et_0

#### memory alloc process

- take size from malloc & calc how many blocks we need.
- check entry table for the first entry that has a type of HEAP_BLOCK_TABLE_ENTRY_FREE.
- if for example we need 8192 we also need to find an entry with a free block next to it.
- once we have the two we mark those as taken
- we now return the abs address that the start block represents. Calculation: (heap_data_start_addr + (block_number \* block_size))

## Paging

- allows to remap memory addresses to point to other memory addresses
- can be used to provide the illusion that you have the maximum amount of ram installed
- can be used to hide memory from other processes

### remapping memory

- allows to remap one mem addr to another. So 0x100000 could point to 0x200000
- paging works in 4096 blocks by default. The blocks are called pages
- when

### benefits

- each process can access the same virtual memory addresses, never writing over eachother
- security.
- can be used to prevent overwriting sensitive data such as program code
- etc.

# fat16

- first sector is the **boot sector** on a disk. Fields also exist there that describe the fs such as how many reserved sectors follow this sector
- then there are the **reserved sectors** -> ignored by the fs.
- Now we have our first allocation table, it contains values that represent which clusters on the disk are taken and which are free. (Cluster = certain number of sectors joined together)
- Next file allocation table, it's optional though (as backup)
- Now comes the root directory, which explains what files/dirs are in root. Each entry has a relative name that represents the file or dir name, attributes such as read only, the address of the first cluster representing the data, etc.
- finally, we have our **data region**. all the data is here.

# kernel land

- kernel land when the processor is in its maximum privileged state
- whilst in kernel land any area in memory can be changed
- any cpu instruction can be run
- also a rist of damage to the system if an error occurs persists
- kernel land is when the processor is in a privileged protection ring such as ring 0

# user land

- limited processor state
- generally, processes run in user land
- safe bc if something goes wrong the kernel is able to intervene
- cpu is in ring 3
- its **NOT** a special place where processes run, its just a term to describe the processor when its in a privilege limited state

### getting to user land

1. setup user code and data segments
2. setup a tts(task switch segment)
3. pretend we are returning from an interrupt pushing appropriate flags, and data to the stack before executing an "iret" to change the cpu priv state.

### communication with the kernel from a process

#### user program calls interrupt using the interrupt instruction

Let's pretend we have a k-op that's represented by code 1 (print a message to the screen)

1. user program sets the eax reg to 1 (k-op for print operations)
2. user program pushes the address of the message that should be printed to the screen
3. user program issues an interrupt to the kernel. Interrupt number used is 0x80 bc in this hypothetical example we decided to use 0x80 for handling commands to the kernel.
