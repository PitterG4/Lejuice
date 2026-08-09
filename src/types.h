#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>

#define MAX_CMD_LEN 8191
#define MAX_PATH_LEN 4096
#define MAX_FILENAME_LEN 256
#define MAX_PARAMS_LEN 64
#define MAX_EXT_LEN 16

// Watcher.c
typedef struct {
    char old_filepath[MAX_PATH_LEN];
    char old_filename[MAX_FILENAME_LEN];
    char new_filepath[MAX_PATH_LEN];
    char new_filename[MAX_FILENAME_LEN];
} FileEvent;

// Parser.c
typedef struct {
    char old_ext[MAX_EXT_LEN];
    char new_ext[MAX_EXT_LEN];
    char inline_params[MAX_PARAMS_LEN];
    
    bool is_valid_change;
} FileInfo;

// Config.c
typedef struct {
    char key[MAX_PARAMS_LEN];
    char value[MAX_CMD_LEN];
} Macro;

typedef struct {
    char name[16];
    Macro *macros;
} ToolConfig;

typedef struct {
    char from_ext[MAX_EXT_LEN];         
    char to_ext[MAX_EXT_LEN];
    char command_template[MAX_CMD_LEN];
    ToolConfig *tool;
} ConversionRule;

#endif