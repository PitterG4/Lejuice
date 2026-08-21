#ifndef TYPES_H
#define TYPES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#define MAX_CMD_LEN 8191
#define MAX_PATH_LEN 4096
#define MAX_FILENAME_LEN 256
#define MAX_PARAMS_LEN 64
#define MAX_EXT_LEN 16

typedef enum {
    SUCCESS = 0,

    // File error
    ERR_FILE_NOT_FOUND,
    ERR_FILE_READ,

    // Memory error
    ERR_NO_MEMORY,

    // Configuration error
    ERR_BAD_SYNTAX,
    ERR_MISSING_TOOL,

    // Processing error
    ERR_INVALID_PARAM,
    ERR_UNSUPPORTED_FORMAT
} ReturnCode;

// Watcher.c
typedef struct {
    char old_filepath[MAX_PATH_LEN];
    char new_filepath[MAX_PATH_LEN];
    char old_filename[MAX_FILENAME_LEN];
    char new_filename[MAX_FILENAME_LEN];
} FileEvent;

// Parser.c
typedef struct {
    char old_ext[MAX_EXT_LEN];
    char new_ext[MAX_EXT_LEN];
    char inline_params[MAX_PARAMS_LEN];
} FileInfo;

// Config.c
typedef struct {
    char key[MAX_PARAMS_LEN];
    char value[MAX_CMD_LEN];
} Macros;

typedef struct {
    Macros *macros;
    char name[MAX_FILENAME_LEN];
    int num_macros;
} Tools;

typedef struct {
    Tools *tool;
    char command_template[MAX_CMD_LEN];
    char tool_name_temp[MAX_FILENAME_LEN];
    char from_ext[MAX_EXT_LEN];         
    char to_ext[MAX_EXT_LEN];
} Rules;

// Mallocs for the structs
typedef struct {
    Rules *rules;
    Tools *tools;
    Macros *macros;
    char watch_dir[MAX_PATH_LEN];
    int num_rules;
    int num_tools;
    int num_macros;
} Config;

#endif