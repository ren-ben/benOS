FILES=./build/start.o ./build/benos.o ./build/stdlib.o
INCLUDES=-I./src
FLAGS= -g

all: ${FILES}
	i686-elf-ld -m elf_i386 -relocatable ${FILES} -o ./stdlib.elf

./build/start.o: ./src/start.asm
	nasm -f elf32 ./src/start.asm -o ./build/start.o

./build/benos.o: ./src/benos.asm
	nasm -f elf32 ./src/benos.asm -o ./build/benos.o

./build/stdlib.o: ./src/stdlib.c
	i686-elf-gcc ${INCLUDES} $(FLAGS) -std=gnu99 -c ./src/stdlib.c -o ./build/stdlib.o


clean:
	rm -rf ${FILES}
	rm ./stdlib.elf