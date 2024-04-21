#include "microcli.h"
#include <string.h>

// Special characters:
#define ESC 0x1b
#define SEQ 0x5b
#define DEL 0x7f

// End of line:
#define EOL "\r\n"

static void insert_spaces(MicroCLI_t * ctx, unsigned int num)
{
    for(unsigned int i = 0; i < num; i++) {
        ctx->cfg.printf(" ");
    }
}

static unsigned int cmd_len(const char * cmdStr)
{
    unsigned int inLen = strlen(cmdStr);
    unsigned int i;
    for(i = 0; i < inLen; i++) {
        if(cmdStr[i] == ' ')
            break;
    }
    return i;
}

static int lookup_command(MicroCLI_t * ctx)
{
    // Loop through each possible command
    int i = ctx->cfg.cmdCount;
    while(i-- > 0) {
        // Compare the input command (delimited by a space) to the command in the table
        bool diff = false;
        unsigned int j = strlen(ctx->cfg.cmdTable[i].name);
        if(j != cmd_len(ctx->input.buffer))
            continue;
        while(j-- > 0 && !diff) {
            if(ctx->input.buffer[j] != ctx->cfg.cmdTable[i].name[j])
                diff = true;
        }
        if(!diff)
            return i;
    }
    return MICROCLI_ERR_CMD_NOT_FOUND;
}

void microcli_prompt_for_input(MicroCLI_t * ctx)
{
    // Only display prompt once per command
    if(ctx->prompted == false) {
        ctx->cfg.printf(ctx->cfg.promptText);
        ctx->prompted = true;
    }
}

int microcli_handle_char(MicroCLI_t * ctx, char ch)
{
    // Convenience mapping
    char *buffer = ctx->input.buffer;

    if (!ctx->input.ready) {
        // Handle termination characters
        if(ch == 0 || ch == '\n' || ch == '\r') {
            ctx->input.ready = true;
            return 0;
        }

        // Handle escape sequence
        if (ctx->input.len > 1 &&
            buffer[ctx->input.len - 2] == ESC &&
            buffer[ctx->input.len - 1] == SEQ) {
            // Drop the sequence from the input buffer
            buffer[--ctx->input.len] = 0;
            buffer[--ctx->input.len] = 0;
            return 0;
        }

        // Handle backspace
        if (ch == '\b' || ch == DEL) {
            if (ctx->input.len > 0) {
                // Overwrite prev char from screen
                ctx->cfg.printf("\b \b");
                ctx->input.len--;
            }

            // Erase from buffer
            buffer[ctx->input.len] = 0;
            return 0;
        }

        // Handle normal character
        if (ctx->input.len < MICROCLI_MAX_INPUT_LEN) {
            // Echo back if not an escape sequence
            if (ch != ESC && (ctx->input.len < 1 || buffer[ctx->input.len - 1] != ESC))
                ctx->cfg.printf("%c", ch);
            ctx->input.buffer[ctx->input.len++] = ch;
        } else
            return MICROCLI_ERR_BUFFER_FULL;
    }
    return 0;
}

int microcli_execute_command(MicroCLI_t * ctx) {
    if (!ctx->input.ready)
        return MICROCLI_ERR_BUSY;

    // New line
    ctx->cfg.printf(EOL);

    // Replace escape character with null terminator
    ctx->input.buffer[ctx->input.len] = 0;

    // Command entry is complete. Input buffer is ready to be processed
    // Lookup and run command (if found)
    int cmdRet;
    int cmdIdx = lookup_command(ctx);
    if (cmdIdx == MICROCLI_ERR_CMD_NOT_FOUND)
        cmdRet = MICROCLI_ERR_CMD_NOT_FOUND;
    else {
        if(cmdIdx >= 0 && cmdIdx < ctx->cfg.cmdCount) {
            ctx->cfg.printf(EOL);
            char * args = ctx->input.buffer + cmd_len(ctx->input.buffer) + 1;
            cmdRet = ctx->cfg.cmdTable[cmdIdx].cmd(ctx, args);
        } else
            cmdRet = MICROCLI_ERR_CMD_NOT_FOUND;
    }

    // Reset input state.
    memset(&ctx->input, 0, sizeof(ctx->input));

    // Reset prompt
    ctx->prompted = false;

    return cmdRet;
}

void microcli_init(MicroCLI_t * ctx, const MicroCLICfg_t * cfg)
{
    memset(ctx, 0, sizeof(MicroCLI_t));
    ctx->cfg = *cfg;
}

int microcli_banner(MicroCLI_t * ctx)
{
    return ctx->cfg.printf(ctx->cfg.bannerText);
}

int microcli_help(MicroCLI_t * ctx)
{
    int printLen = 0;
    for(int i = 0; i < ctx->cfg.cmdCount; i++) {
        int cmdLen = ctx->cfg.printf("%s", ctx->cfg.cmdTable[i].name);
        insert_spaces(ctx, MICROCLI_MAX_CMD_LEN - cmdLen);
        printLen += MICROCLI_MAX_CMD_LEN;
        printLen += ctx->cfg.printf("%s" EOL, ctx->cfg.cmdTable[i].help);
    }
    return printLen;
}
