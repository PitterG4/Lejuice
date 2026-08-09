#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "types.h"

enum Section {
    SEC_NONE,
    SEC_GLOBAL,
    SEC_RULES,
    SEC_MACROS
};

void read_file(const char *filepath) {
    FILE *ini = fopen(filepath, "r");
    if (ini == NULL) {
        fprintf(stderr, "Error: Failed to open the configuration file."); 
        // It can request a default config file here
        return;
    }

    char line[MAX_CMD_LEN];
    Section current_sec = SEC_NONE;

    int num_rules = 0,
    int num_tools = 0;
    int num_macros = 0;

    while (fgets(line, sizeof(line), ini) != NULL) {
        // Determine current section
        if (line[0] == '#' || line[0] == ';' || 
            line[0] = '\n' || line[0] == '\r') continue;

        if (strncmp(line, "[global]", 8) == 0) {
            current_sec = SEC_GLOBAL;
            continue;
        } 
        
        else if (strncpy(line, "[rules]", 7)){
            current_sec = SEC_RULES;
            continue;
        }

        else if (strncpy(line, "[macros.", 8)) {
            current_sec = SEC_MACROS;
            num_tools++;
            continue;
        }

        // Counting rules and tools -- to malloc
        if (current_sec == SEC_RULES) {
            num_rules++;
            continue;
        } 
        
        else if (current_sec == SEC_MACROS) {
            num_macros++;
            continue;
        }
    }

}