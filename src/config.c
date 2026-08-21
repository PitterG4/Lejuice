#include "types.h"

typedef enum section {
    SEC_NONE,
    SEC_GLOBAL,
    SEC_RULES,
    SEC_MACROS
} Section;

// Function to Qsort
int comp_str(const void *a, const void *b) {
    const Rules *rule_a = (const Rules *)a;
    const Rules *rule_b = (const Rules *)b;

    const int cmp = strcmp(rule_a->from_ext, rule_b->from_ext);
    if (cmp != 0) return cmp;

    return strcmp(rule_a->to_ext, rule_b->to_ext);
}

void free_config_struct(Config config) {
    free(config.rules);
    free(config.tools);
    free(config.macros);
}

// State machine
Section section_search(const char *line, Section current) {
    if (strncmp(line, "[global]", 8) == 0) return SEC_GLOBAL;
    if (strncmp(line, "[rules]", 7) == 0) return SEC_RULES;
    if (strncmp(line, "[macros.", 8) == 0) return SEC_MACROS;
    return current;
}

Config load_config(const char *filepath, ReturnCode *status) {
    Config config = {0};

    FILE *ini = fopen(filepath, "r");
    if (ini == NULL) {
        fprintf(stderr, "Error: Failed to open the configuration file.");
        return config;
    }

    char line[MAX_CMD_LEN];
    Section current_sec = SEC_NONE;

    int num_rules = 0;
    int num_tools = 0;
    int num_macros = 0;

    // 1st loop: count rules, tools and macros -- to allocate memory later
    while (fgets(line, sizeof(line), ini) != NULL) {
        if (line[0] == '#' || line[0] == ';' ||
            line[0] == '\n' || line[0] == '\r') continue;

        if (line[0] == '[') {
            current_sec = section_search(line, current_sec);

            if (strncmp(line, "[macros.", 8) == 0) num_tools++;
             continue;
        }

        if (current_sec == SEC_RULES) num_rules++;
        else if (current_sec == SEC_MACROS) num_macros++;
    }

    // Pointers to move through allocated memory
    Rules *current_rule = NULL;
    Tools *current_tool = NULL;
    Macros *current_macro = NULL;

    // All counted, time to allocate
    if (num_rules > 0) {
        config.num_rules = num_rules;
        config.rules = malloc(num_rules * sizeof(Rules));
        current_rule = config.rules;
    }

    if (num_tools > 0) {
        config.num_tools = num_tools;
        config.tools = malloc(num_tools * sizeof(Tools));
        current_tool = config.tools;
    }

    if (num_macros > 0) {
        config.num_macros = num_macros;
        config.macros = malloc(num_macros * sizeof(Macros));
        current_macro = config.macros;
    }

    rewind(ini);
    current_sec = SEC_NONE;

    // 2nd loop: fill the data
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

        if (current_sec == SEC_MACROS && current_tool > config.tools) {
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
                    (current_tool - 1)->num_macros++;

                    current_macro++;
                }
            }
        }
    }

    // Connect rules pointer to tools
    for (int i = 0; i < num_rules; i++) {
        config.rules[i].tool = NULL;

        for (int j = 0; j < num_tools; j++) {
            if (strcmp(config.rules[i].tool_name_temp, config.tools[j].name) == 0) {
                config.rules[i].tool = &config.tools[j];
                break;
            }
        }
    }

    qsort(config.rules, num_rules, sizeof(Rules), comp_str);
    fclose(ini);

    return config;
}
