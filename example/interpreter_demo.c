#include "microcli.h"
#include <stdio.h>
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

#define CMD_ENTRY(fn, help) {fn, #fn, help}

int poke(MicroCLI_t * ctx, const char * args);
int help(MicroCLI_t * ctx, const char * args);

MicroCLI_t dbg;
const MicroCLICmdEntry_t cmdTable[] = {
    CMD_ENTRY(help, "Print this help message"),
    CMD_ENTRY(poke, "Poke the poor circuit"),
};
const MicroCLICfg_t dbgCfg = {
    .printf = printf,
    .bannerText = "\r\n\n\n\nMicroCLI Interpreter Demo\r\n",
    .promptText = "> ",
    .cmdTable = cmdTable,
    .cmdCount = sizeof(cmdTable)/sizeof(cmdTable[0]),
};

int poke(MicroCLI_t * ctx, const char * args)
{
    microcli_log(ctx, "ouch!\n\r");
    return 0;
}

int help(MicroCLI_t * ctx, const char * args)
{
    return microcli_help(ctx);
}

int main(int argc, char* argv[])
{
    microcli_init(&dbg, &dbgCfg);
    microcli_banner(&dbg);
    while(true) {
        prompt_for_input(&dbg);
        int ch = getch();

        if (ch == 224) {
            handle_char(&dbg, '\033');
            handle_char(&dbg, '[');
            switch (getch()) {
                case 72:
                    handle_char(&dbg, 'A');
                    break;
                case 80:
                    handle_char(&dbg, 'B');
                    break;
                default:
                    handle_char(&dbg, ch);
                    break;
            }
        } else {
            handle_char(&dbg, ch);
        }

        if (execute_command(&dbg) == MICROCLI_ERR_CMD_NOT_FOUND) {
            microcli_warn(&dbg, "Command not found!\r\n");
        }
    }
}
