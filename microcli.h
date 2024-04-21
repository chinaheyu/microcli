#ifndef _MICROCLI_H_
#define _MICROCLI_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "microcli_config.h"
#include <stdbool.h>

// Error codes
typedef enum {
    MICROCLI_ERR_UNKNOWN = -128,
    MICROCLI_ERR_BUSY,
    MICROCLI_ERR_BUFFER_FULL,
    MICROCLI_ERR_CMD_NOT_FOUND,
    MICROCLI_ERR_MAX
} MicroCLIErr_t;

// Typedefs
typedef struct microcliCmdEntry     MicroCLICmdEntry_t;
typedef struct microcliCfg          MicroCLICfg_t;
typedef struct microcliCtx          MicroCLI_t;

// Command function prototype
typedef int (*MicroCLICmd_t)(MicroCLI_t * ctx, char * args);

// I/O function prototypes
typedef int (*Printf_t)(const char * fmt, ...);
typedef int (*Getchar_t)(void);

struct microcliCmdEntry {
    MicroCLICmd_t cmd;
    const char * name;
    const char * help;
};

struct microcliCfg {
    Printf_t printf;
    const char * bannerText;
    const char * promptText;
    const MicroCLICmdEntry_t * cmdTable;
    unsigned short cmdCount;
};

struct microcliCtx {
    MicroCLICfg_t cfg;
    bool prompted;
    struct {
        char buffer[MICROCLI_MAX_INPUT_LEN + 1];
        unsigned int len;
        bool ready;
    } input;
};

// Control
void microcli_init(MicroCLI_t * ctx, const MicroCLICfg_t * cfg);
int microcli_execute_command(MicroCLI_t * ctx);

// Input
int microcli_handle_char(MicroCLI_t * ctx, char ch);

// Output
int microcli_banner(MicroCLI_t * ctx);
int microcli_help(MicroCLI_t * ctx);
void microcli_prompt_for_input(MicroCLI_t * ctx);
#define microcli_printf(ctx, fmt, ...) ((ctx)->cfg.printf(fmt, ##__VA_ARGS__))

// Helper
#define CMD_ENTRY(fn, help) {fn, #fn, help}

#ifdef __cplusplus
}
#endif

#endif /* _MICROCLI_H_ */
