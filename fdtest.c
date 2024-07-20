#include <stdio.h>
#include <stdlib.h>
#include "byos.h"
#include "byos.c"

#define NUM_COMMANDS 1000

int main(void)
{
    char *cmd_args[] = { "echo", "Hello", NULL };
    struct cmd cmds[NUM_COMMANDS];
    
    for (int i = 0; i < NUM_COMMANDS; ++i) {
        cmds[i].redir_stdout = NULL;
        cmds[i].type = FORX;
        cmds[i].data.forx.pathname = "echo";
        cmds[i].data.forx.argv = cmd_args;
    }
    
    struct cmd example = {
        .redir_stdout = NULL,
        .type = LIST,
        .data = { .list = { NUM_COMMANDS, cmds } }
    };

    int r = interp(&example);
    printf("return value = %d\n", r);

    return 0;
}
