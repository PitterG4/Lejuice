#pragma once

#include <stdbool.h>
#include <stdint.h>

#define BUFFER_SIZE 8192
#define MAX_CMD_LEN 8191
#define MAX_PATH_LEN 4096
#define MAX_FILENAME_LEN 256
#define MAX_PARAMS_LEN 128
#define MAX_EXT_LEN 16

#define MIN_FREE_SIZE_MB 128

#define DEFAULT_QUEUE_SIZE 8
#define DEFAULT_ESCAPE_PREFIX "!"

extern struct Config config;

// to main.c
typedef enum {
    CMD_RUN,    CMD_STOP,
    CMD_WATCH,  CMD_SETUP,
    CMD_STATUS, CMD_CHECK,
    CMD_HELP,   CMD_UNKNOWN,
} Command;

typedef enum tag {
    TAG_NONE,
    TAG_DIR,        // 'C:\videos\'

    TAG_IN,         // 'C:\videos\video.mp4'
    TAG_IN_NAME,    // 'video.mp4'
    TAG_IN_BASE,    // 'video'
    TAG_IN_EXT,     // 'mp4'

    TAG_OUT,        // 'C:\videos\muted video.mov'
    TAG_OUT_NAME,   // 'muted video.mov'
    TAG_OUT_BASE,   // 'muted video'
    TAG_OUT_EXT,    // 'mov'

    TAG_ARGS,       // '-an -vf scale=-1:1080'  -- processed params
    TAG_PARAMS,     // 'mute -vf scale=-1:1080' -- raw params
    TAG_INSTALL,    // Documents\Lejuice
} Tag;

typedef enum {
    SEC_NONE,
    SEC_WATCHER, SEC_PARSER,
    SEC_ACTIONS, SEC_EVENTS,
    SEC_RULES,   SEC_MACROS,
} Section;

typedef enum {
    FLAG_SYS_INFO       = 1 << 0,   // 0000 0001
    FLAG_SYS_ERROR      = 1 << 1,   // 0000 0010

    FLAG_TASK_START     = 1 << 2,   // 0000 0100
    FLAG_TASK_DONE      = 1 << 3,   // 0000 1000
    FLAG_TASK_ERROR     = 1 << 4,   // 0001 0000

    FLAG_FILTER_RULE    = 1 << 5,   // 0010 0000
    FLAG_FILTER_NOISE   = 1 << 6,   // 0100 0000

    FLAG_DEBUG          = 1 << 7,   // 1000 0000

    FLAG_SYS     = (FLAG_SYS_INFO | FLAG_SYS_ERROR),
    FLAG_TASK    = (FLAG_TASK_START | FLAG_TASK_DONE | FLAG_TASK_ERROR),
    FLAG_FILTER  = (FLAG_FILTER_RULE | FLAG_FILTER_NOISE),

    FLAG_DEFAULT = (FLAG_SYS | FLAG_TASK_ERROR),
} EventFlag;

// REP_NONE/UNK_NONE is -4 to handle errors
// If not treated it'll become 0 (default) when converted to uint32_t 2 bits
typedef enum {
    REP_TRASH       = 0,
    REP_KEEP        = 1,
    REP_ASK         = 2,

    REP_NONE        = -4,
} ReplaceAction;

typedef enum {
    MAC_STRICT      = 0,
    MAC_PREFIX      = 1,
    MAC_SUFFIX      = 2,

    MAC_NONE         = -4,
} MacroAction;

typedef struct {
    // Watcher output to parser
    char old_filepath[MAX_PATH_LEN];
    char new_filepath[MAX_PATH_LEN];
    char old_filename[MAX_FILENAME_LEN];
    char new_filename[MAX_FILENAME_LEN];

    // Parser output
    char old_ext[MAX_EXT_LEN];
    char new_ext[MAX_EXT_LEN];

    char out_filepath[MAX_PATH_LEN];

    char old_base_name[MAX_FILENAME_LEN];
    char new_base_name[MAX_FILENAME_LEN];
    char inline_params[MAX_PARAMS_LEN];

    int old_dir_len;
    int new_dir_len;
} FileEvent;

typedef struct {
    FileEvent *events;
    int size;
    int head;
    int tail;
} EventQueue;

typedef struct {
    char filename[MAX_FILENAME_LEN];
    uint64_t expire_time;
} CooldownItem;

typedef struct {
    CooldownItem *items;
    int tail;
    int size;
} CooldownQueue;

typedef struct {
    // Events
    uint32_t notify         : 8;
    uint32_t log            : 8;

    // Actions
    uint32_t replace        : 2;
    uint32_t macro_match    : 2;

    uint32_t is_local       : 1;
    uint32_t reserved       : 9;
} Profile;

typedef struct {
    char key[MAX_PARAMS_LEN];
    char value[MAX_CMD_LEN];

    char key_1st[MAX_PARAMS_LEN / 2];
    char key_2nd[MAX_PARAMS_LEN / 2];
    bool has_var;

    struct Tools *override;
} Macros;

typedef struct Tools {
    Macros *macros;
    char name[MAX_FILENAME_LEN];

    // Macros info
    uint16_t num_macros_local;
    uint16_t num_pure;
    uint16_t num_prefix;

    bool has_override;
    bool has_wildcard;
} Tools;

typedef struct {
    Tools *tool;
    char command_template[MAX_CMD_LEN];
    char tool_name_temp[MAX_FILENAME_LEN];
    char from_ext[MAX_EXT_LEN];         
    char to_ext[MAX_EXT_LEN];

    Profile local;
} Rules;

typedef struct {
    int num_rules;
    int num_tools;
    int num_macros;
    int line_error;
} ConfigCount;

typedef struct Config {
    Rules *rules;
    Tools *tools;
    Macros *macros;
    ConfigCount count;

    char install_path[MAX_PATH_LEN];
    char watch_dir[MAX_PATH_LEN];
    char escape_prefix[MAX_EXT_LEN];
    int queue_size;

    Profile global;
} Config;