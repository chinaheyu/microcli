#include "microcli.h"
#include <string.h>

// End of line:
#define EOL "\r\n"

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

static void reset_input_state(MicroCLI_t * ctx) {
    // Reset input buffer.
    memset(&ctx->input, 0, sizeof(ctx->input));

    // Reset prompt
    ctx->prompted = false;
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
            // New line
            ctx->cfg.printf(EOL);
            // Command entry is complete. Input buffer is ready to be processed
            ctx->input.ready = true;
            return MICROCLI_ERR_SUCCESS;
        }

        // Handle backspace
        if (ch == '\b' || ch == 0x7f) {
            if (ctx->input.len > 0) {
                // Overwrite prev char from screen
                ctx->cfg.printf("\b \b");
                ctx->input.len--;
            }

            // Erase from buffer
            buffer[ctx->input.len] = 0;
            return MICROCLI_ERR_SUCCESS;
        }

        // Handle normal character (printable ascii)
        if (ch >= 0x20 && ch <= 0x7e) {
            if (ctx->input.len < MICROCLI_MAX_INPUT_LEN) {
                // Save to input buffer
                ctx->input.buffer[ctx->input.len++] = ch;
                // Echo back
                ctx->cfg.printf("%c", ch);
                return MICROCLI_ERR_SUCCESS;
            } else
                return MICROCLI_ERR_BUFFER_FULL;
        }
    }

    // Invalid character
    return MICROCLI_ERR_INVALID_CHARACTER;
}

int microcli_execute_command(MicroCLI_t * ctx) {
    if (!ctx->input.ready)
        return MICROCLI_ERR_BUSY;

    if (!ctx->input.len) {
        reset_input_state(ctx);
        return MICROCLI_ERR_EMPTY_INPUT;
    }

    // Lookup and run command (if found)
    int cmdRet;
    int cmdIdx = lookup_command(ctx);
    if(cmdIdx >= 0 && cmdIdx < ctx->cfg.cmdCount) {
        char * args = ctx->input.buffer + cmd_len(ctx->input.buffer) + 1;
        cmdRet = ctx->cfg.cmdTable[cmdIdx].cmd(ctx, args);
    } else
        cmdRet = MICROCLI_ERR_CMD_NOT_FOUND;

    reset_input_state(ctx);

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
        for(int j = 0; j < MICROCLI_MAX_CMD_LEN - cmdLen; j++) {
            ctx->cfg.printf(" ");
        }
        printLen += MICROCLI_MAX_CMD_LEN;
        printLen += ctx->cfg.printf("%s" EOL, ctx->cfg.cmdTable[i].help);
    }
    return printLen;
}
