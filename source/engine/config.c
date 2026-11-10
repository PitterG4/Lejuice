#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "types.h"

#define STARTS_WITH(text, prefix) (strncmp(text, prefix, sizeof(prefix) - 1) == 0)

Config config = {0};

void free_config_struct()
{
    free(config.rules);
    free(config.tools);
    free(config.macros);
}

void set_config_default()
{
    char install_path[MAX_PATH_LEN];
    memcpy(install_path, config.install_path, sizeof(install_path));

    memset(&config, 0, sizeof(Config));
    memcpy(config.install_path, install_path, sizeof install_path);

    snprintf(config.escape_prefix, sizeof(config.escape_prefix), "%s", DEFAULT_ESCAPE_PREFIX);
    config.queue_size       = DEFAULT_QUEUE_SIZE;

    config.global.log       = FLAG_DEFAULT;
    config.global.notify    = FLAG_DEFAULT;
}

Section section_search(const char *line, Section current)
{
    if (STARTS_WITH(line, "[watcher]"))         return SEC_WATCHER;

    if (STARTS_WITH(line, "[actions.global]") ||
        STARTS_WITH(line, "[actions]"))         return SEC_ACTIONS;

    if (STARTS_WITH(line, "[events.global]") ||
        STARTS_WITH(line, "[events]"))          return SEC_EVENTS;

    if (STARTS_WITH(line, "[rules]"))           return SEC_RULES;

    if (STARTS_WITH(line, "[macros."))          return SEC_MACROS;

    return current;
}

uint32_t set_event_flag(char *value)
{
    uint32_t mask = 0;
    char *token = strtok(value, " ,");

    while (token != NULL) {
        if      (strcmp(token, "all") == 0) return ~0;
        if      (strcmp(token, "none") == 0) return 0;
        if      (strcmp(token, "default") == 0) mask |= FLAG_DEFAULT;

        // System category
        if      (strcmp(token, "system") == 0) mask |= FLAG_SYS;
        else if (strcmp(token, "system_info") == 0) mask |= FLAG_SYS_INFO;
        else if (strcmp(token, "system_error") == 0) mask |= FLAG_SYS_ERROR;

        // Task category
        if      (strcmp(token, "task") == 0) mask |= FLAG_TASK;
        else if (strcmp(token, "task_start") == 0) mask |= FLAG_TASK_START;
        else if (strcmp(token, "task_done") == 0)  mask |= FLAG_TASK_DONE;
        else if (strcmp(token, "task_error") == 0) mask |= FLAG_TASK_ERROR;

        // Filter category
        if      (strcmp(token, "filter") == 0) mask |= FLAG_FILTER;
        else if (strcmp(token, "filter_rule") == 0) mask |= FLAG_FILTER_RULE;
        else if (strcmp(token, "filter_noise") == 0) mask |= FLAG_FILTER_NOISE;

        token = strtok(NULL, " ,");
    }

    return mask;
}
ReplaceAction set_replace_tag(const char *token)
{
    if (strcmp(token, "trash") == 0)        return REP_TRASH;
    if (strcmp(token, "keep") == 0)         return REP_KEEP;
    if (strcmp(token, "ask") == 0)          return REP_ASK;

    return REP_NONE;
}
MacroAction set_macro_tag(const char *token)
{
    if (strcmp(token, "strict") == 0)       return MAC_STRICT;
    if (strcmp(token, "prefix") == 0)       return MAC_PREFIX;
    if (strcmp(token, "suffix") == 0)       return MAC_SUFFIX;

    return MAC_NONE;
}

bool set_properties(const char *key, char *value, Rules *rule)
{
    Profile *target;

    if (rule == NULL) {
        target = &config.global;

        // Category: watcher
        if (strcmp(key, "watch_dir") == 0) {
            snprintf(config.watch_dir, MAX_PATH_LEN, "%s", value);
            return true;
        }
        if (strcmp(key, "queue_size") == 0) {
            config.queue_size = strtol(value, NULL, 10);
            return true;
        }
        if (strcmp(key, "escape_prefix") == 0) {
            snprintf(config.escape_prefix, MAX_EXT_LEN, "%s", value);
            return true;
        }
    }

    else (target = &rule->local);

    // Category: actions
    if (strcmp(key, "replace") == 0) {
        target->replace = set_replace_tag(value);
        return true;
    }
    if (strcmp(key, "macro_match") == 0) {
        target->macro_match = set_macro_tag(value);
        return true;
    }

    // Category: events
    if (strcmp(key, "notify") == 0) {
        target->notify = set_event_flag(value);
        return true;
    }
    if (strcmp(key, "log") == 0) {
        target->log = set_event_flag(value);
        return true;
    }

    return false;
}

bool parse_property_line(char *line, Rules *rule)
{
    char *equal = strchr(line, '=');
    if (equal == NULL) return false;

    *equal = '\0';
    char *key_part = line;
    char *value_part = equal + 1;

    while (*value_part == ' ' || *value_part == '\t') value_part++;

    value_part[strcspn(value_part, "\r\n")] = '\0';
    key_part[strcspn(key_part, " \t")] = '\0';

    return set_properties(key_part, value_part, rule);
}

bool parse_rule_line(char *line, Rules *rule)
{
    char *colon = strchr(line, ':');
    if (colon == NULL) return false;

    *colon = '\0';
    char *ext_part = line;
    char *cmd_part = colon + 1;

    if (sscanf(ext_part, " %15s -> %15s", rule->from_ext, rule->to_ext) == 2) {
        while (*cmd_part == ' ' || *cmd_part == '\t') cmd_part++;
        cmd_part[strcspn(cmd_part, "\r\n")] = '\0';

        snprintf(rule->command_template, MAX_CMD_LEN, "%s", cmd_part);
        sscanf(cmd_part, "%s", rule->tool_name_temp);

        return true;
    }

    return false;
}

bool parse_macro_line(char *line, Macros *macro)
{
    char *equal = strchr(line, '=');
    if (equal == NULL) return false;

    *equal = '\0';

    char *key_part = line;
    char *value_part = equal + 1;

    while (*value_part == ' ' || *value_part == '\t') value_part++;
    value_part[strcspn(value_part, "\r\n")] = '\0';
    key_part[strcspn(key_part, " \t")] = '\0';

    if (*key_part == '\0' || *value_part == '\0') return false;

    char *brace = strstr(key_part, "{VAR}");
    if (brace != NULL) {
        macro->has_var = true;
        int len_1st = (int)(brace - key_part);

        snprintf(macro->key_1st, MAX_PARAMS_LEN / 2, "%.*s", len_1st, key_part);
        snprintf(macro->key_2nd, MAX_PARAMS_LEN / 2, "%s", brace + 5);
    } else {
        macro->has_var = false;
    }

    snprintf(macro->key, MAX_PARAMS_LEN, "%s", key_part);
    snprintf(macro->value, MAX_CMD_LEN, "%s", value_part);
    return true;
}

// 1st pass function
ConfigCount count_and_validate(FILE *ini)
{
    ConfigCount count = {0};
    char line[MAX_CMD_LEN];
    Section current_section = SEC_NONE;
    int current_line = 0;

    while (fgets(line, sizeof(line), ini) != NULL) {
        current_line++;

        char *line_start = line;
        while (*line_start == ' ' || *line_start == '\t') line_start++;

        if (line_start[0] == '#'    || line_start[0] == ';' ||
            line_start[0] == '\n'   || line_start[0] == '\r' ||
            line_start[0] == '\0')  continue;

        if (line_start[0] == '[') {
            if (strchr(line_start, ']') == NULL) {
                count.line_error = current_line;
                return count;
            }
            current_section = section_search(line_start, current_section);

            if ((current_section == SEC_WATCHER || current_section == SEC_ACTIONS ||current_section == SEC_EVENTS)
                && count.num_rules > 0) {
                count.line_error = current_line;
                return count;
            }

            if (STARTS_WITH(line_start, "[macros.")) count.num_tools++;
            continue;
        }

        if (current_section == SEC_WATCHER || current_section == SEC_ACTIONS ||
            current_section == SEC_EVENTS  || current_section == SEC_MACROS) {
            if (strchr(line_start, '=') == NULL && line_start[0] != '@') {
                count.line_error = current_line;
                return count;
            }
            if (current_section == SEC_MACROS) {
                count.num_macros++;
            }
        }
        else if (current_section == SEC_RULES) {
            if (line_start[0] == '>') {
                if (count.num_rules == 0 || strchr(line_start, '=') == NULL) {
                    count.line_error = current_line;
                    return count;
                }
                continue;
            }

            char *arrow = strstr(line_start, "->");
            char *colon = strchr(line_start, ':');

            if (arrow == NULL || colon == NULL || (colon < arrow)) {
                count.line_error = current_line;
                return count;
            }
            count.num_rules++;
        }
    }

    return count;
}

// Sorting stuff ---> start here

int comp_rules(const void *a, const void *b)
{
    const Rules *rule_a = (const Rules*) a;
    const Rules *rule_b = (const Rules*) b;

    const int cmp = strcmp(rule_a->from_ext, rule_b->from_ext);
    if (cmp != 0) return cmp;

    return strcmp(rule_a->to_ext, rule_b->to_ext);
}

int comp_tools(const void *a, const void *b)
{
    const Tools *tool_a = (const Tools*) a;
    const Tools *tool_b = (const Tools*) b;

    return strcmp(tool_a->name, tool_b->name);
}

int group_macro(const Macros *macro)
{
    if (!macro->has_var)          return 0; // Pure: xy

    bool has_prefix = macro->key_1st[0] != '\0';
    bool has_suffix = macro->key_2nd[0] != '\0';

    if (has_prefix && has_suffix) return 1; // Mixed: x{VAR}y
    if (has_prefix)               return 2; // Prefix: x{VAR}
    if (has_suffix)               return 3; // Suffix: {VAR}y

    return 4; // Only variable: {VAR}
}
int comp_half(const char *a, const char *b)
{
    size_t len_a = strlen(a);
    size_t len_b = strlen(b);

    // Sort by length
    if (len_a > len_b) return -1;
    if (len_a < len_b) return 1;

    // Sort alphabetically
    return strcmp(a, b);
}
int comp_macros(const void *a, const void *b)
{
    const Macros *macro_a = (const Macros*) a;
    const Macros *macro_b = (const Macros*) b;

    int group_a = group_macro(macro_a);
    int group_b = group_macro(macro_b);

    if (group_a < group_b) return -1;
    if (group_a > group_b) return 1;

    if (group_a == 0) return strcmp(macro_a->key, macro_b->key);

    int cmp = comp_half(macro_a->key_1st, macro_b->key_1st);
    if (cmp != 0) return cmp;

    return comp_half(macro_a->key_2nd, macro_b->key_2nd);
}

void sort_config_file()
{
    // Sort every rule
    qsort(config.rules, config.count.num_rules, sizeof(Rules), comp_rules);

    // Sort every tool
    qsort(config.tools, config.count.num_tools, sizeof(Tools), comp_tools);

    for (int i = 0; i < config.count.num_tools; i++) {
        if (config.tools[i].num_macros_local < 1) continue;

        // Sort every tool's macros
        qsort(config.tools[i].macros, config.tools[i].num_macros_local, sizeof(Macros), comp_macros);
    }
}

// Sorting stuff ---> finish here

Tools *find_tool_name(const char *name)
{
    if (name == NULL || name[0] == '\0') return NULL;

    Tools found_tool = {0};
    snprintf(found_tool.name, MAX_FILENAME_LEN, "%s", name);

    return bsearch(&found_tool, config.tools, config.count.num_tools, sizeof(Tools), comp_tools);
}

void link_pointers()
{
    // link rules
    for (int i = 0; i < config.count.num_rules; i++) {
        config.rules[i].tool = find_tool_name(config.rules[i].tool_name_temp);
    }

    // link macro overrides
    for (int i = 0; i < config.count.num_tools; i++) {
        Tools *tool = &config.tools[i];

        if (!tool->has_override) continue;

        for (int j = 0; j < tool->num_macros_local; j++) {
            Macros *current_macro = &tool->macros[j];

            if (current_macro->value[0] == '@') {
                char *space= current_macro->value + 1;
                int len = (int)strcspn(space, " \t");

                char override_name[MAX_FILENAME_LEN] = {0};
                snprintf(override_name, MAX_FILENAME_LEN, "%.*s", len, space);

                current_macro->override = find_tool_name(override_name);
            }
        }
    }
}

bool load_config(const char *filepath)
{
    set_config_default();

    FILE *ini = fopen(filepath, "r");
    if (ini == NULL) return false;

    char line[MAX_CMD_LEN];
    Section current_section = SEC_NONE;

    // 1st loop: count rules, tools and macros -- to allocate memory later
    config.count = count_and_validate(ini);
    if (config.count.line_error > 0) {
        fclose(ini);
        return false;
    }

    // Pointers to move through allocated memory
    Rules *current_rule = NULL;
    Tools *current_tool = NULL;
    Macros *current_macro = NULL;

    // All counted, time to allocate
    if (config.count.num_rules > 0) {
        config.rules = calloc(config.count.num_rules, sizeof(Rules));
        current_rule = config.rules;
    }

    if (config.count.num_tools > 0) {
        config.tools = calloc(config.count.num_tools, sizeof(Tools));
        current_tool = config.tools;
    }

    if (config.count.num_macros > 0) {
        config.macros = calloc(config.count.num_macros, sizeof(Macros));
        current_macro = config.macros;
    }

    rewind(ini);
    current_section = SEC_NONE;

    // 2nd loop: fill the data
    while (fgets(line, sizeof(line), ini) != NULL) {
        char *line_start = line;
        while (*line_start == ' ' || *line_start == '\t') line_start++;

        if (line_start[0] == '#'    || line_start[0] == ';' ||
            line_start[0] == '\n'   || line_start[0] == '\r' ||
            line_start[0] == '\0')  continue;

        if (line_start[0] == '[') {
            current_section = section_search(line_start, current_section);

            if (current_section == SEC_MACROS) {
                // Found a new tool
                char *final_bracket = strrchr(line_start, ']');
                if (final_bracket != NULL) *final_bracket = '\0';

                if (current_tool->name[0] != '\0') current_tool++;

                snprintf(current_tool->name, MAX_FILENAME_LEN, "%s", line_start + (sizeof("[macros.") - 1));
                current_tool->macros = current_macro;
            }
            continue;
        }

        if (current_section == SEC_WATCHER || current_section == SEC_ACTIONS || current_section == SEC_EVENTS) {
            parse_property_line(line_start, NULL);
        }

        else if (current_section == SEC_RULES) {
            if (line_start[0] == '>' && current_rule > config.rules) {
                Rules *last_rule = current_rule - 1;

                if (!last_rule->local.is_local) {
                    last_rule->local = config.global;
                    last_rule->local.is_local = true;
                }

                line_start++;
                while (*line_start == ' ' || *line_start == '\t') line_start++;

                parse_property_line(line_start, last_rule);
            }

            else if (parse_rule_line(line_start, current_rule)) {
                current_rule->local = config.global;
                current_rule++;
            }
        }

        else if (current_section == SEC_MACROS && current_tool->name[0] != '\0') {
            if (parse_macro_line(line_start, current_macro)) {
                current_tool->num_macros_local++;

                if (!current_macro->has_var) current_tool->num_pure++;
                else if (current_macro->key_1st[0] != '\0') current_tool->num_prefix++;
                else if (current_macro->key_2nd[0] == '\0') current_tool->has_wildcard = true;

                if (current_macro->value[0] == '@') current_tool->has_override = true;

                current_macro++;
            }
        }
    }

    sort_config_file();
    link_pointers();

    fclose(ini);
    return true;
}