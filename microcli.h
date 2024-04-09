#ifndef _MICROCLI_H_
#define _MICROCLI_H_

#ifdef __cplusplus
#ifndef _Static_assert
    #define _Static_assert static_assert
#endif
extern "C" {
#endif

#include "microcli_config.h"
#include "microcli_verbosity.h"
#include <stdbool.h>


// Error codes
typedef enum {
    MICROCLI_ERR_UNKNOWN = -128,
    MICROCLI_ERR_BUSY,
    MICROCLI_ERR_BAD_ARG,
    MICROCLI_ERR_OVERFLOW,
    MICROCLI_ERR_OUT_OF_BOUNDS,
    MICROCLI_ERR_CMD_NOT_FOUND,
    MICROCLI_ERR_NO_DATA,
    MICROCLI_ERR_MAX
} MicroCLIErr_t;
_Static_assert(MICROCLI_ERR_MAX < 0, "Some MicroCLI error codes are non-negative!");

// Typedefs
typedef struct microcliCmdEntry     MicroCLICmdEntry_t;
typedef struct microcliCfg          MicroCLICfg_t;
typedef struct microcliCtx          MicroCLI_t;

// Command function prototype
typedef int (*MicroCLICmd_t)(MicroCLI_t * ctx, const char * args);

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
    unsigned int cmdCount;
};

struct microcliCtx {
    MicroCLICfg_t cfg;
    int verbosity;
    bool prompted;
    struct {
        char buffer[MAX_CLI_INPUT_LEN + 1];
        unsigned int len;
        bool ready;
    } input;
#ifdef MICROCLI_ENABLE_HISTORY
    char history[MICRICLI_MAX_HISTORY][MAX_CLI_INPUT_LEN + 1];
    unsigned int historyHead;
    unsigned int historyTail;
    unsigned int historyEntry;
    bool historySelected;
#endif
};
#ifdef MICROCLI_ENABLE_HISTORY
_Static_assert(MICRICLI_MAX_HISTORY >= 2, "At least 2 history!");
#endif

// Control
void microcli_init(MicroCLI_t * ctx, const MicroCLICfg_t * cfg);
void microcli_set_verbosity(MicroCLI_t * ctx, int verbosity);
int execute_command(MicroCLI_t * ctx);

// Input
int handle_char(MicroCLI_t * ctx, char ch);

// Output
int microcli_banner(MicroCLI_t * ctx);
int microcli_help(MicroCLI_t * ctx);
void prompt_for_input(MicroCLI_t * ctx);
#define microcli_error(ctx, fmt, ...) MICROCLI_ERROR(ctx, fmt, ##__VA_ARGS__)
#define microcli_warn(ctx, fmt, ...) MICROCLI_WARN(ctx, fmt, ##__VA_ARGS__)
#define microcli_log(ctx, fmt, ...) MICROCLI_LOG(ctx, fmt, ##__VA_ARGS__)
#define microcli_debug(ctx, fmt, ...) MICROCLI_DEBUG(ctx, fmt, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* _MICROCLI_H_ */