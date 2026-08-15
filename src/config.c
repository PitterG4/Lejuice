#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "types.h"

typedef enum section {
    SEC_NONE,
    SEC_GLOBAL,
    SEC_RULES,
    SEC_MACROS
} Section;

Section section_search(char *line, Section current) {
    if (line[0] == '#' || line[0] == ';' || 
        line[0] == '\n' || line[0] == '\r') return current;
    if (strncmp(line, "[global]", 8) == 0) return SEC_GLOBAL;
    if (strncmp(line, "[rules]", 7) == 0) return SEC_RULES;
    if (strncmp(line, "[macros.", 8) == 0) return SEC_MACROS;
    return current;
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

    int num_rules = 0;
    int num_tools = 0;
    int num_macros = 0;

    while (fgets(line, sizeof(line), ini) != NULL) {
        current_sec = section_search(line, current_sec);
        if (line[0] == '[') continue;

        // Counting rules and tools -- to malloc
        if (current_sec == SEC_RULES) {
            num_rules++;
            continue;
        }
        
        else if (current_sec == SEC_MACROS) {
            num_macros++;
            continue;
        }

        else if (strncmp(line, "[macros.", 8) == 0) {
            num_tools++;
            num_macros--;
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
        current_sec = section_search(line, current_sec);
        if (line[0] == '[') continue;

        if (current_sec == SEC_RULES) {
            char *semicolon = strchr(line, ':');
            
            if (semicolon != NULL) {
                *semicolon = '\0';

                char *cmd_part = semicolon + 1;
                char *ext_part = line;

                if (ext_part == NULL || cmd_part == NULL) {
                    fprintf(stderr, "Error: Configuration file formatted incorrectly.");
                    return;
                }

                if (sscanf(ext_part, " %s -> %s", 
                    current_rule->from_ext, current_rule->to_ext) == 2) {
                        while (*cmd_part == ' ' || *cmd_part == '\t') cmd_part++;
                        snprintf(current_rule->command_template, MAX_CMD_LEN, "%s", cmd_part);

                        current_rule++;
                }
            }
        }


    }
}
