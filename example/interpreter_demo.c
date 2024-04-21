#include "microcli.h"
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
int getch()
{
    struct termios old, current;
    tcgetattr(0, &old);
    current = old;
    current.c_lflag &= ~ICANON;
    current.c_lflag &= ~ECHO;
    tcsetattr(0, TCSANOW, &current);
    int ch = getchar();
    tcsetattr(0, TCSANOW, &old);
    return ch;
}
#endif

int poke(MicroCLI_t * ctx, char * args);
int echo(MicroCLI_t * ctx, char * args);
int help(MicroCLI_t * ctx, char * args);

MicroCLI_t dbg;
const MicroCLICmdEntry_t cmdTable[] = {
    CMD_ENTRY(poke, "Poke the poor circuit"),
    CMD_ENTRY(echo, "Echo arguments"),
    CMD_ENTRY(help, "Print this help message")
};
const MicroCLICfg_t dbgCfg = {
    .printf = printf,
    .bannerText = "\r\n\n\n\nMicroCLI Interpreter Demo\r\n",
    .promptText = "> ",
    .cmdTable = cmdTable,
    .cmdCount = sizeof(cmdTable)/sizeof(cmdTable[0]),
};

int poke(MicroCLI_t * ctx, char * args)
{
    microcli_printf(ctx, "ouch!\n\r");
    return 0;
}

int echo(MicroCLI_t * ctx, char * args)
{
    int cnt = 0;
    char *pch = strtok(args," ");
    while (pch != NULL)
    {
        microcli_printf(ctx, "%d: %s\n", cnt++, pch);
        pch = strtok(NULL, " ");
    }
    return 0;
}

int help(MicroCLI_t * ctx, char * args)
{
    return microcli_help(ctx);
}

int main(int argc, char* argv[])
{
    microcli_init(&dbg, &dbgCfg);
    microcli_banner(&dbg);
    for(;;) {
        microcli_prompt_for_input(&dbg);
        int ch = getch();

        // Drop Windows key scan code
        if (ch == 224) {
            getch();
            continue;
        }

        microcli_handle_char(&dbg, (char)ch);

        if (microcli_execute_command(&dbg) == MICROCLI_ERR_CMD_NOT_FOUND) {
            microcli_printf(&dbg, "Command not found!\r\n");
        }
    }
}
