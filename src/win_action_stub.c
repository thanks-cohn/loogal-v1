#include <stdio.h>

int cmd_action(int argc, char **argv) {
    (void)argc;
    (void)argv;
    puts("action command not available in quick Windows build");
    return 1;
}
