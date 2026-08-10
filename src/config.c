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

void section_search(char *line) {
    if (line[0] == '#' || line[0] == ';' || 
        line[0] = '\n' || line[0] == '\r') {
            continue;
    }

    if (strncmp(line, "[global]", 8) == 0) {
        current_sec = SEC_GLOBAL;
        continue;
    }
    
    else if (strncmp(line, "[rules]", 7) == 0){
        current_sec = SEC_RULES;
        continue;
    }

    else if (strncmp(line, "[macros.", 8) == 0) {
        current_sec = SEC_MACROS;
        continue;
    }
}

void read_file(char *filepath) {
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
        section_search(line);

        // Counting rules and tools -- to malloc
        if (current_sec == SEC_RULES) {
            num_rules++;
            continue;
        }
        
        else if (current_sec == SEC_MACROS) {
            num_macros++;
            continue;
        }

        else if (strncpy(line, "[macros.", 8) == 0) {
            num_macros++;
            continue;
        }
    }

    ConversionRule *rules = malloc(num_rules * sizeof(ConversionRule));
    ConversionRule *current_rule = rules;
    ToolConfig *tools = malloc(num_tools * sizeof(ToolConfig));
    Macro *macros = malloc(num_macros * sizeof(Macro));

    rewind(ini);
    current_sec = SEC_NONE;

    while (fgets(line, sizeof(line), ini) != NULL) {
        section_search(line);

        if (current_sec == SEC_RULES) {
            if (line[0] == '[') continue;

            char *ext_part = strtok(line, ":");
            char *cmd_part = strtok(NULL, ":");

            if (ext_part == NULL || cmd_part == NULL) {
                fprintf(stderr, "Error: Configuration file formatted incorrectly.");
                return;
            }

            ext_part = strtok(ext_part, " ->"); // ps: this ignores extensions as jpeg-2000
            snprintf(current_rule->from_ext, MAX_EXT_LEN, "%s", ext_part);

            ext_part = strtok(NULL, " ->");
            snprintf(current_rule->to_ext, MAX_EXT_LEN, "%s", ext_part);

            if (*cmd_part == ' ') {
                cmd_part++;
            }

            snprintf(current_rule->command_template, MAX_CMD_LEN, "%s", cmd_part);
            current_rule++;
        }
    }
}
