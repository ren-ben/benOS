#include "benos.h"
#include "string.h"

struct command_arg* benos_parse_command(const char* command, int max) {
    struct command_arg* root_command = 0;
    // command buffer
    char scommand[1025];
    if (max >= (int) sizeof(scommand)) {
        return 0;
    }

    strncpy(scommand, command, sizeof(scommand));
    char* token = strtok(scommand, " ");
    if (!token) {
        goto out;
    }

    root_command = benos_malloc(sizeof(struct command_arg));
    if (!root_command) {
        goto out;
    }

    strncpy(root_command->arg, token, sizeof(root_command->arg));
    root_command->next = 0;

    struct command_arg* current = root_command;
    token = strtok(NULL, " ");
    while(token != 0) {
        struct command_arg* new_command = benos_malloc(sizeof(struct command_arg));
        if (!new_command) {
            break;
        }

        strncpy(new_command->arg, token, sizeof(new_command->arg));
        new_command->next = 0;
        current->next = new_command;
        current = new_command;
        token = strtok(NULL, " ");
    }

out:
    return root_command;
}

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

int benos_system_run(const char* command) {
    char buf[1024];
    strncpy(buf, command, sizeof(buf));
    struct command_arg* root_command_arg = benos_parse_command(buf, sizeof(buf));
    if (!root_command_arg) {
        return -1;
    }

    return benos_system(root_command_arg);;
}