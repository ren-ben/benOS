#include "shell.h"
#include "../../stdlib/src/stdio.h"
#include "../../stdlib/src/stdlib.h"
#include "../../stdlib/src/benos.h"

int main(int argc, char** argv)
{
    print("\nBenos v1.0.0\n\n");
    while(1) {
        print("benos> ");
        char buf[1024];
        benos_terminal_readline(buf, sizeof(buf), true);
        benos_process_load_start(buf);
        print("\n");
    }
    return 0;
}
