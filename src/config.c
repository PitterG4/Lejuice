#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "types.h"

int comp_str(const void *a, const void *b) {
    const ConversionRule *rule_a = (const ConversionRule *)a;
    const ConversionRule *rule_b = (const ConversionRule *)b;

    int cmp = strcmp(rule_a->from_ext, rule_b->from_ext);
    if (cmp != 0) return cmp;

    return strcmp(rule_a->to_ext, rule_b->to_ext);
}

typedef enum section {
    SEC_NONE,
    SEC_GLOBAL,
    SEC_RULES,
    SEC_MACROS
} Section;

Section section_search(char *line, Section current) {
    if (strncmp(line, "[global]", 8) == 0) return SEC_GLOBAL;
    if (strncmp(line, "[rules]", 7) == 0) return SEC_RULES;
    if (strncmp(line, "[macros.", 8) == 0) return SEC_MACROS;
    return current;
}

void read_file(char *filepath) {
    FILE *ini = fopen(filepath, "r");
    if (ini == NULL) {
        fprintf(stderr, "Error: Failed to open the configuration file."); 
        // TODO: It can request a default template of config file here
        return;
    }

    char line[MAX_CMD_LEN];
    Section current_sec = SEC_NONE;

    int num_rules = 0;
    int num_tools = 0;
    int num_macros = 0;

    while (fgets(line, sizeof(line), ini) != NULL) {
        if (line[0] == '#' || line[0] == ';' ||
            line[0] == '\n' || line[0] == '\r') continue;

        if (line[0] == '[') {
            current_sec = section_search(line, current_sec, NULL);

            if (strncmp(line, "[macros.", 8) == 0) {
            num_tools++;
            } continue;
        }

        // Counting rules and macros -- to malloc
        if (current_sec == SEC_RULES) {
            num_rules++;
        }

        else if (current_sec == SEC_MACROS) {
            num_macros++;
            continue;
        }
    }

    // All counted, time to allocate
    ConversionRule *rules = malloc(num_rules * sizeof(ConversionRule));
    ToolConfig *tools = malloc(num_tools * sizeof(ToolConfig));
    Macro *macros = malloc(num_macros * sizeof(Macro));

    // Pointers to move through allocated memory
    ConversionRule *current_rule = rules;
    ToolConfig *current_tool = tools;
    Macro *current_macro = macros;

    rewind(ini);
    current_sec = SEC_NONE;

    while (fgets(line, sizeof(line), ini) != NULL) {
        if (line[0] == '#' || line[0] == ';' ||
            line[0] == '\n' || line[0] == '\r') continue;

        if (line[0] == '[') {
            current_sec = section_search(line, current_sec);
            
            if (current_sec == SEC_MACROS) {
                char *final_bracket = strrchr(line, ']');
                if (final_bracket != NULL) {
                    *final_bracket = '\0';

                    snprintf(current_tool->name, MAX_FILENAME_LEN, "%s", line + 8);

                    current_tool->macros = current_macro;
                    current_tool->num_macros = 0;
                    current_tool++;
                }
            }
            continue;
        }

        if (current_sec == SEC_RULES) {
            char *colon = strchr(line, ':');
            
            if (colon != NULL) {
                *colon = '\0';

                char *ext_part = line;
                char *cmd_part = colon + 1;

                cmd_part[strcspn(cmd_part, "\r\n")] = '\0';

                if (sscanf(ext_part, " %s -> %s", current_rule->from_ext, current_rule->to_ext) == 2) {
                    while (*cmd_part == ' ' || *cmd_part == '\t') cmd_part++;

                    snprintf(current_rule->command_template, MAX_CMD_LEN, "%s", cmd_part);
                    sscanf(cmd_part, "%s", current_rule->tool_name_temp);

                    current_rule++;
                }
            }
        }

        if (current_sec == SEC_MACROS) {
            char *equal = strchr(line, '=');

            if (equal != NULL) {
                *equal = '\0';

                char *key_part = line;
                char *value_part = equal + 1;

                while (*value_part == ' ' || *value_part == '\t') value_part++;
                value_part[strcspn(value_part, "\r\n")] = '\0';
                key_part[strcspn(key_part, " \t")] = '\0';

                if (*key_part != '\0' && *value_part != '\0') {
                    snprintf(current_macro->key, MAX_PARAMS_LEN, "%s", key_part);
                    snprintf(current_macro->value, MAX_PARAMS_LEN, "%s", value_part);

                    current_macro++;
                    (current_tool - 1)->num_macros++;
                }
            }
        }
    }

    // Connect rules pointer to tools
    for (int i = 0; i < num_rules; i++) {
        rules[i].tool = NULL;

        for (int j = 0; j < num_tools; j++) {
            if (strcmp(rules[i].tool_name_temp, tools[j].name) == 0) {
                rules[i].tool = &tools[j];
                break;
            }
        }
    }

    qsort(rules, num_rules, sizeof(ConversionRule), comp_str);
    fclose(ini);
}