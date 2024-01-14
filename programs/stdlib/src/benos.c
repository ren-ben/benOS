#include "benos.h"

int benos_getkeyblock() {
    int val = 0;
    do {
        val = benos_getkey();
    } while (val == 0);
    return val;
}

// out_while_typing (false) is mainly for password input
void benos_terminal_readline(char* out, int max, bool out_while_typing) {
    int i = 0;
    for (i = 0; i < max - 1; i++) {
        char key = benos_getkeyblock();

        //carriage return means we have read the line
        if (key == 13) {
            break;
        }

        if (out_while_typing) {
            benos_putchar(key);
        }

        //backspace
        if (key == 0x08 && i >= 1) {
            out[i-1] = 0x00;
            // -2 because we want to remove the backspace character as well
            i -= 2;
            continue;
        }

        out[i] = key;
    }

    // add null terminator
    out[i] = 0x00;
} 