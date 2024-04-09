#include "microcli.h"
#include <stdbool.h>
#include <string.h>
#include <assert.h>


// Special characters:
enum {
    ESC = 27,
    UP_ARROW = 65,
    DOWN_ARROW = 66,
    SEQ = 91,
    DEL = 127
};

static void insert_spaces(MicroCLI_t * ctx, int num)
{
    assert(ctx);
    assert(num > 0);
    
    for(int i = 0; i < num; i++) {
        ctx->cfg.printf(" ");
    }
}

static inline int cmd_len(const char * cmdStr)
{
    assert(cmdStr);

    int inLen = strlen(cmdStr);
    int i = 0;
    for(i = 0; i < inLen; i++) {
        if(cmdStr[i] == ' ')
            break;
    }

    return i;
}

static int lookup_command(MicroCLI_t * ctx)
{
    assert(ctx);
    assert(ctx->cfg.cmdTable);
    assert(ctx->cfg.cmdCount >= 0);

    // Loop through each possible command
    int i = ctx->cfg.cmdCount;
    while(i-- > 0) {
        // Compare the input command (delimited by a space) to
        // the command in the table
        bool diff = false;
        int j = strlen(ctx->cfg.cmdTable[i].name);
        if(j > cmd_len(ctx->input.buffer))
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

static inline void save_input_to_history(MicroCLI_t * ctx)
{
    #ifdef MICROCLI_ENABLE_HISTORY
        assert(ctx);
        assert(ctx->historyHead < MICRICLI_MAX_HISTORY && ctx->historyTail < MICRICLI_MAX_HISTORY); // Memory overflow?
        assert(ctx->input.len <= MAX_CLI_INPUT_LEN); // Input overflow?
        assert(ctx->input.buffer[ctx->input.len] == 0); // Null terminated?
        
        // Abort if there is no data to save
        if(ctx->input.len <= 1)
            return;

        // Save the input string
        ctx->historyTail = (ctx->historyTail + 1) % MICRICLI_MAX_HISTORY;
        if(ctx->historyTail == ctx->historyHead)
            ctx->historyHead = (ctx->historyHead + 1) % MICRICLI_MAX_HISTORY;
        strcpy(ctx->history[ctx->historyTail], ctx->input.buffer);
        ctx->historyEntry = ctx->historyTail;
        ctx->historySelected = false;
    #endif
}

inline void prompt_for_input(MicroCLI_t * ctx)
{
    assert(ctx);
    assert(ctx->cfg.printf);
    assert(ctx->cfg.promptText);

    // Only display prompt once per command
    if(ctx->prompted == false) {
        ctx->cfg.printf(ctx->cfg.promptText);
        ctx->prompted = true;
    }
}

static inline void clear_line(MicroCLI_t * ctx)
{
    assert(ctx);
    assert(ctx->cfg.printf);
    
    ctx->cfg.printf("\r%c%c%c", ESC, SEQ, 'K');
}

static inline void clear_prompt(MicroCLI_t * ctx)
{
    assert(ctx);
    assert(ctx->cfg.printf);
    assert(ctx->cfg.promptText);
    
    clear_line(ctx);
    ctx->cfg.printf("\r%s", ctx->cfg.promptText);
}

static inline void print_history_entry( MicroCLI_t * ctx )
{
    assert(ctx);
    assert(ctx->cfg.printf);
    assert(ctx->historyEntry < MICRICLI_MAX_HISTORY);

    // Clear the line
    clear_prompt(ctx);

    // Print the history entry text
    ctx->cfg.printf("%s", ctx->history[ctx->historyEntry]);
}

int handle_char(MicroCLI_t * ctx, char ch)
{
    assert(ctx);
    assert(ctx->cfg.printf);

    // Convenience mapping
    char *buffer = ctx->input.buffer;

    if (!ctx->input.ready) {
        // Handle termination characters
        if( ch == 0 || ch == '\n' || ch == '\r' ) {
            ctx->input.ready = true;
            return 0;
        }

        // Handle escape sequence
        if (ctx->input.len > 1 &&
            buffer[ctx->input.len - 2] == ESC &&
            buffer[ctx->input.len - 1] == SEQ) {
            // Clear the sequence from the input buffer
            buffer[--ctx->input.len] = 0;
            buffer[--ctx->input.len] = 0;

            // Handle the special character
            switch (ch) {
                case UP_ARROW:
                    if (!ctx->historySelected) {
                        ctx->historyEntry = ctx->historyTail;
                        ctx->historySelected = true;
                    } else if (ctx->historyHead < ctx->historyTail && ctx->historyEntry > ctx->historyHead) {
                        ctx->historyEntry--;
                        ctx->historySelected = true;
                    } else if (ctx->historyHead > ctx->historyTail &&
                               (ctx->historyEntry <= ctx->historyTail || ctx->historyEntry > ctx->historyHead)) {
                        ctx->historyEntry = ctx->historyEntry > 0 ? ctx->historyEntry - 1 : MICRICLI_MAX_HISTORY - 1;
                        ctx->historySelected = true;
                    }
                    break;
                case DOWN_ARROW:
                    if (ctx->historyHead < ctx->historyTail && ctx->historyEntry < ctx->historyTail) {
                        ctx->historyEntry++;
                        ctx->historySelected = true;
                    } else if (ctx->historyHead > ctx->historyTail &&
                               (ctx->historyEntry < ctx->historyTail || ctx->historyEntry >= ctx->historyHead)) {
                        ctx->historyEntry = (ctx->historyEntry + 1) % MICRICLI_MAX_HISTORY;
                        ctx->historySelected = true;
                    } else {
                        // Reset back to input buffer
                        ctx->historySelected = false;
                        clear_prompt(ctx);
                        memset(&ctx->input, 0, sizeof(ctx->input));
                    }
                    break;
                default:
                    break;
            }
            if (ctx->historySelected) {
                print_history_entry(ctx);
                strcpy(ctx->input.buffer, ctx->history[ctx->historyEntry]);
                ctx->input.len = strnlen(ctx->history[ctx->historyEntry], MAX_CLI_INPUT_LEN);
            }
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
        } else {
            if (ctx->input.len < MAX_CLI_INPUT_LEN) {
                // Echo back if not an escape sequence
                if (ch != ESC && (ctx->input.len < 1 || buffer[ctx->input.len - 1] != ESC))
                    ctx->cfg.printf("%c", ch);
                ctx->input.buffer[ctx->input.len++] = ch;
            } else
                return MICROCLI_ERR_OUT_OF_BOUNDS;
        }
    }
}

int execute_command(MicroCLI_t * ctx) {
    assert(ctx);
    assert(ctx->cfg.printf);
    if (!ctx->input.ready)
        return MICROCLI_ERR_BUSY;

    // Replace escape character with null terminator
    ctx->input.buffer[ctx->input.len] = 0;
    ctx->cfg.printf("\r\n");

    // Command entry is complete. Input buffer is ready to be processed
    assert(ctx->input.len <= MAX_CLI_INPUT_LEN);
    save_input_to_history(ctx);

    // Lookup and run command (if found)
    int cmdRet;
    int cmdIdx = lookup_command(ctx);
    if (cmdIdx == MICROCLI_ERR_CMD_NOT_FOUND)
        cmdRet = MICROCLI_ERR_CMD_NOT_FOUND;
    else {
        if(cmdIdx >= 0 && cmdIdx < ctx->cfg.cmdCount) {
            ctx->cfg.printf("\r\n");
            const char * args = ctx->input.buffer + cmd_len(ctx->input.buffer) + 1;
            cmdRet = ctx->cfg.cmdTable[cmdIdx].cmd(ctx, args);
        } else
            cmdRet = MICROCLI_ERR_OUT_OF_BOUNDS;
    }

    // Reset input state.
    memset(&ctx->input, 0, sizeof(ctx->input));

    // Reset prompt
    ctx->prompted = false;

    return cmdRet;
}

void microcli_init(MicroCLI_t * ctx, const MicroCLICfg_t * cfg)
{
    assert(ctx);
    assert(cfg);
    assert(cfg->printf);
    assert(cfg->bannerText);
    assert(cfg->promptText);
    assert(cfg->cmdTable);

    *ctx = (MicroCLI_t){0};

    ctx->cfg = *cfg;
    ctx->verbosity = DEFAULT_VERBOSITY;
}

void microcli_set_verbosity(MicroCLI_t * ctx, int verbosity)
{
    assert(ctx);
    assert(verbosity <= VERBOSITY_LEVEL_MAX);
    ctx->verbosity = verbosity;
}

int microcli_banner(MicroCLI_t * ctx)
{
    assert(ctx);
    assert(ctx->cfg.bannerText);
    assert(ctx->cfg.printf);
    
    return ctx->cfg.printf(ctx->cfg.bannerText);
}

int microcli_help(MicroCLI_t * ctx)
{
    assert(ctx);
    assert(ctx->cfg.printf);
    assert(ctx->cfg.cmdTable);
    assert(ctx->cfg.cmdCount >= 0);

    int printLen = 0;
    for(int i = 0; i < ctx->cfg.cmdCount; i++) {
        int cmdLen = ctx->cfg.printf("%s", ctx->cfg.cmdTable[i].name);
        insert_spaces(ctx, MAX_CLI_CMD_LEN - cmdLen);
        printLen += MAX_CLI_CMD_LEN;
        printLen += ctx->cfg.printf("%s\n\r", ctx->cfg.cmdTable[i].help);
    }
    
    return printLen;
}
