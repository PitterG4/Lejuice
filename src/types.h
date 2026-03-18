#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>

#define MAX_PATH_LEN 4096
#define MAX_FILENAME_LEN 256
#define MAX_EXT_LEN 16
#define MAX_PARAMS_LEN 64
#define MAX_CMD_LEN 512

typedef enum {
    EVENT_UNDEFINED,
    EVENT_RENAMED,
    EVENT_CREATED,
    EVENT_DELETED
} EventType;

// Watcher.c
typedef struct {
    char old_filepath[MAX_PATH_LEN];
    char new_filepath[MAX_PATH_LEN];
    EventType type;
} FileEvent;

// Parser.c
typedef struct {
    char base_name[MAX_FILENAME_LEN];       // Nome limpo -- ok
    char old_ext[MAX_EXT_LEN];          // Extensão antiga -- ok
    char new_ext[MAX_EXT_LEN];          // Extensão nova -- ok
    char inline_params[MAX_PARAMS_LEN]; // O que estava nas chaves
    
    bool has_escape;        // [TIRAR DAQUI]
    bool is_valid_change;   // true se a extensão realmente mudou
} FilenameInfo;

// Config.c
typedef struct {
    char from_ext[MAX_EXT_LEN];         
    char to_ext[MAX_EXT_LEN];
    char command_template[MAX_CMD_LEN];

    char escape_seq[16];
    bool make_backup;
    char backup_dir[MAX_PATH_LEN];
} ConversionRule;

#endif