#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "system.h"
#include "types.h"

#define STARTS_WITH(text, prefix) (strncmp(text, prefix, sizeof(prefix) - 1) == 0)

Config config = {0};

void free_config_struct()
{
    free(config.rules);
    free(config.tools);
    free(config.macros);

    config.rules  = NULL;
    config.tools  = NULL;
    config.macros = NULL;
}

void set_config_default()
{
    char install_path[MAX_PATH_LEN];
    memcpy(install_path, config.install_path, sizeof(install_path));

    memset(&config, 0, sizeof(Config));
    memcpy(config.install_path, install_path, sizeof install_path);

    snprintf(config.escape_prefix, sizeof(config.escape_prefix), "%s", DEFAULT_ESCAPE_PREFIX);
    config.queue_size       = DEFAULT_QUEUE_SIZE;

    config.param_separator = SEP_DEFAULT;
    config.macro_match     = MAC_STRICT;

    config.global.log       = FLAG_DEFAULT;
    config.global.notify    = FLAG_DEFAULT;
}

char *trim(char *str)
{
    while (*str == ' ' || *str == '\t') str++;

    size_t len = strlen(str);
    while (len > 0 && (str[len - 1] == ' ' || str[len - 1] == '\t' ||
                       str[len - 1] == '\r' || str[len - 1] == '\n')) {str[--len] = '\0';}

    return str;
}

char *skip_ignorable(char *line)
{
    while (*line == ' ' || *line == '\t') line++;

    if (*line == '#' || *line == ';' || *line == '\r' ||
        *line == '\n' || *line == '\0') return NULL;

    return line;
}

Section section_search(const char *line)
{
    if (STARTS_WITH(line, "[watcher]"))         return SEC_WATCHER;
    if (STARTS_WITH(line, "[parser]"))          return SEC_PARSER;

    if (STARTS_WITH(line, "[actions.global]") ||
        STARTS_WITH(line, "[actions]"))         return SEC_ACTIONS;

    if (STARTS_WITH(line, "[events.global]") ||
        STARTS_WITH(line, "[events]"))          return SEC_EVENTS;

    if (STARTS_WITH(line, "[rules]"))           return SEC_RULES;

    if (STARTS_WITH(line, "[macros."))          return SEC_MACROS;

    return SEC_NONE;
}

bool set_event_flag(char *value, uint8_t *out_mask)
{
    uint8_t mask = 0;
    char *token = strtok(value, " ,");
    if (token == NULL) return false;

    while (token != NULL) {
        if      (strcmp(token, "all") == 0) {*out_mask = ~0; return true;}
        else if (strcmp(token, "none") == 0) {*out_mask = 0; return true;}
        else if (strcmp(token, "default") == 0) mask |= FLAG_DEFAULT;

        // System category
        else if (strcmp(token, "system") == 0) mask |= FLAG_SYS;
        else if (strcmp(token, "system_info") == 0) mask |= FLAG_SYS_INFO;
        else if (strcmp(token, "system_error") == 0) mask |= FLAG_SYS_ERROR;

        // Task category
        else if (strcmp(token, "task") == 0) mask |= FLAG_TASK;
        else if (strcmp(token, "task_start") == 0) mask |= FLAG_TASK_START;
        else if (strcmp(token, "task_done") == 0)  mask |= FLAG_TASK_DONE;
        else if (strcmp(token, "task_error") == 0) mask |= FLAG_TASK_ERROR;

        // Filter category
        if      (strcmp(token, "filter") == 0) mask |= FLAG_FILTER;
        else if (strcmp(token, "filter_rule") == 0) mask |= FLAG_FILTER_RULE;
        else if (strcmp(token, "filter_noise") == 0) mask |= FLAG_FILTER_NOISE;
        else return false;

        token = strtok(NULL, " ,");
    }

    *out_mask = mask;
    return true;
}
ReplaceAction set_replace_tag(const char *token)
{
    if (strcmp(token, "trash") == 0)        return REP_TRASH;
    if (strcmp(token, "keep") == 0)         return REP_KEEP;
    if (strcmp(token, "ask") == 0)          return REP_ASK;

    return REP_ERROR;
}
FallbackAction set_fallback_tag(const char *token)
{
    if (strcmp(token, "none") == 0)         return FALL_NONE;
    if (strcmp(token, "active") == 0)       return FALL_ACTIVE;
    if (strcmp(token, "original") == 0)     return FALL_ORIGINAL;

    return FALL_ERROR;
}

MacroMatch set_macro_match_tag(const char *token)
{
    if (strcmp(token, "strict") == 0)       return MAC_STRICT;
    if (strcmp(token, "prefix") == 0)       return MAC_PREFIX;
    if (strcmp(token, "suffix") == 0)       return MAC_SUFFIX;

    return MAC_ERROR;
}
ParamSeparator set_separator_tag(char *value)
{
    uint8_t mask = 0;
    char *token = strtok(value, " ,");
    if (token == NULL) return SEP_ERROR;

    while (token != NULL) {
        if (strcmp(token, "space") == 0) mask |= SEP_SPACE;
        else if (strcmp(token, "comma") == 0) mask |= SEP_COMMA;
        else return SEP_ERROR;

        token = strtok(NULL, " ,");
    }

    return mask;
}

bool set_properties(const char *key, char *value, Rules *rule)
{
    Profile *target = &config.global;

    if (rule == NULL) {
        // Category: watcher
        if (strcmp(key, "watch_dir") == 0) {
            if (strlen(value) >= MAX_PATH_LEN) return false;

            snprintf(config.watch_dir, MAX_PATH_LEN, "%s", value);
            return true;
        }
        if (strcmp(key, "queue_size") == 0) {
            char *end;
            int chosen_size = strtol(value, &end, 10);
            if (*end != '\0' || chosen_size <= 0) return false;

            config.queue_size = chosen_size;
            return true;
        }
        if (strcmp(key, "escape_prefix") == 0) {
            if (strlen(value) >= MAX_EXT_LEN) return false;

            snprintf(config.escape_prefix, MAX_EXT_LEN, "%s", value);
            return true;
        }

        // Category: parser
        if (strcmp(key, "macro_match") == 0) {
            MacroMatch chosen_match = set_macro_match_tag(value);
            if (chosen_match == MAC_ERROR) return false;

            config.macro_match = chosen_match;
            return true;
        }
        if (strcmp(key, "param_separator") == 0) {
            ParamSeparator chosen_separator = set_separator_tag(value);
            if (chosen_separator == SEP_ERROR) return false;

            config.param_separator = chosen_separator;
            return true;
        }
    }
    else target = &rule->local;

    // Category: actions
    if (strcmp(key, "replace") == 0) {
        ReplaceAction chosen_replace = set_replace_tag(value);
        if (chosen_replace == REP_ERROR) return false;

        target->replace = chosen_replace;
        return true;
    }
    if (strcmp(key, "fallback") == 0) {
        FallbackAction chosen_fallback = set_fallback_tag(value);
        if (chosen_fallback == FALL_ERROR) return false;

        target->fallback = chosen_fallback;
        return true;
    }

    // Category: events
    if (strcmp(key, "notify") == 0) {
        uint8_t mask;
        if (!set_event_flag(value, &mask)) return false;

        target->notify = mask;
        return true;
    }
    if (strcmp(key, "log") == 0) {
        uint8_t mask;
        if (!set_event_flag(value, &mask)) return false;

        target->log = mask;
        return true;
    }

    return false;
}

bool parse_property_line(char *line, Rules *rule)
{
    char *equal = strchr(line, '=');
    if (equal == NULL) return false;
    *equal = '\0';

    char *key_part   = trim(line);
    char *value_part = trim(equal + 1);

    if (strlen(key_part) == 0 || *value_part == '\0') return false;
    if (strcspn(key_part, " \t") != strlen(key_part)) return false;

    return set_properties(key_part, value_part, rule);
}

bool parse_rule_line(char *line, Rules *rule)
{
    char *arrow = strstr(line, "->");
    char *colon = strchr(line, ':');

    if (arrow == NULL || colon == NULL) return false;
    if (colon < (arrow + 2)) return false;

    *arrow = '\0';
    *colon = '\0';

    char *from_ext  = trim(line);
    char *to_ext    = trim(arrow + 2);
    char *cmd_part  = trim(colon + 1);

    size_t from_len = strlen(from_ext);
    size_t to_len   = strlen(to_ext);
    size_t cmd_len  = strlen(cmd_part);

    size_t tool_len = strcspn(cmd_part, " \t");

    if (from_len == 0 || from_len >= MAX_EXT_LEN || strcspn(from_ext, " \t") != from_len) return false;
    if (to_len   == 0 || to_len   >= MAX_EXT_LEN || strcspn(to_ext, " \t")   != to_len)   return false;
    if (cmd_len  == 0 || cmd_len  >= MAX_CMD_LEN || tool_len >= MAX_FILENAME_LEN)         return false;

    snprintf(rule->from_ext, MAX_EXT_LEN, "%s", from_ext);
    snprintf(rule->to_ext, MAX_EXT_LEN, "%s", to_ext);
    snprintf(rule->command_template, MAX_CMD_LEN, "%s", cmd_part);

    snprintf(rule->tool_name_temp, MAX_FILENAME_LEN, "%.*s", (int)tool_len, cmd_part);

    return true;
}

bool parse_macro_line(char *line, Macros *macro, int line_num)
{
    char *equal = strchr(line, '=');
    if (equal == NULL) return false;
    *equal = '\0';

    char *key_part   = trim(line);
    char *value_part = trim(equal + 1);

    size_t key_len = strlen(key_part);
    size_t val_len = strlen(value_part);

    if (key_len == 0 || key_len >= MAX_PARAMS_LEN) return false;
    if (val_len == 0 || val_len >= MAX_CMD_LEN)    return false;

    // Do not allow separator in the macro itself
    if (strcspn(key_part, " \t,") != key_len) return false;

    char *brace = strstr(key_part, "{VAR}");
    if (brace != NULL) {
        if (strstr(brace + TAG_VAR_LEN, "{VAR}") != NULL) return false;

        macro->has_var = true;
        int len_1st = (int)(brace - key_part);

        snprintf(macro->key_1st, MAX_PARAMS_LEN, "%.*s", len_1st, key_part);
        snprintf(macro->key_2nd, MAX_PARAMS_LEN, "%s", brace + TAG_VAR_LEN);
    } else {
        macro->has_var = false;
    }

    snprintf(macro->key, MAX_PARAMS_LEN, "%s", key_part);
    snprintf(macro->value, MAX_CMD_LEN, "%s", value_part);
    macro->ini_line = line_num;
    return true;
}

// 1st pass function: count items and validate the overall structure
ConfigCount count_and_validate(FILE *ini)
{
    ConfigCount count = {0};
    char line[MAX_CMD_LEN];

    int current_line = 0;
    Section current_section = SEC_NONE;
    bool tool_has_wildcard = false;

    while (fgets(line, sizeof(line), ini) != NULL) {
        current_line++;

        char *line_start = skip_ignorable(line);
        if (line_start == NULL) continue;

        if (line_start[0] == '[') {
            char *close_bracket = strchr(line_start, ']');

            if (close_bracket == NULL || skip_ignorable(close_bracket + 1) != NULL) goto error;

            current_section = section_search(line_start);
            if (current_section == SEC_NONE) goto error;

            if ((current_section == SEC_WATCHER || current_section == SEC_ACTIONS ||
                current_section == SEC_EVENTS   || current_section == SEC_PARSER)
                && count.num_rules > 0) goto error;

            if (current_section == SEC_MACROS) {
                char *tool_name = line_start + strlen("[macros.");
                size_t tool_name_len = close_bracket - tool_name;

                if (tool_name_len == 0 || tool_name_len >= MAX_FILENAME_LEN) goto error;
                if (strcspn(tool_name, " \t") < tool_name_len) goto error;

                tool_has_wildcard = false;
                count.num_tools++;
            }
            continue;
        }

        if (current_section == SEC_WATCHER || current_section == SEC_ACTIONS ||
            current_section == SEC_EVENTS  || current_section == SEC_PARSER  ||
            current_section == SEC_MACROS) {
            char *equal = strchr(line_start, '=');

            if (equal == NULL || skip_ignorable(equal + 1) == NULL) goto error;

            if (current_section == SEC_MACROS) {
                int key_len = (int)(equal - line_start);
                while (key_len > 0 && (line_start[key_len - 1] == ' ' || line_start[key_len - 1] == '\t')) key_len--;
                if (key_len == 0) goto error;

                if (key_len == TAG_VAR_LEN && strncmp(line_start, "{VAR}", TAG_VAR_LEN) == 0) {
                    if (tool_has_wildcard) goto error;
                    tool_has_wildcard = true;
                }

                count.num_macros++;
            }
        }
        else if (current_section == SEC_RULES) {
            if (line_start[0] == '>') {
                char *equal = strchr(line_start, '=');

                if (count.num_rules == 0 || equal == NULL) goto error;
                if (skip_ignorable(equal + 1) == NULL) goto error;
                continue;
            }

            char *arrow = strstr(line_start, "->");
            char *colon = strchr(line_start, ':');
            if (arrow == NULL || colon == NULL || (colon < arrow + 2)) goto error;

            count.num_rules++;
        }
        else goto error;
    }

    return count;

error:
    count.line_error = current_line;
    return count;
}

// 2nd pass: fill the allocated structs and validate values
bool fill_config_data(FILE *ini)
{
    char line[MAX_CMD_LEN];
    Section current_section = SEC_NONE;
    int current_line = 0;

    Rules  *current_rule  = config.rules;
    Tools  *current_tool  = NULL;
    Macros *current_macro = config.macros;

    while (fgets(line, sizeof(line), ini) != NULL) {
        current_line++;

        char *line_start = skip_ignorable(line);
        if (line_start == NULL) continue;

        if (line_start[0] == '[') {
            current_section = section_search(line_start);

            if (current_section == SEC_MACROS) {
                char *close_bracket = strchr(line_start, ']');
                if (close_bracket != NULL) *close_bracket = '\0';

                if (current_tool == NULL) current_tool = config.tools;
                else                      current_tool = current_tool + 1;

                snprintf(current_tool->name, MAX_FILENAME_LEN, "%s", line_start + strlen("[macros."));
                current_tool->macros   = current_macro;
                current_tool->ini_line = current_line;
            }
            continue;
        }

        if (current_section == SEC_WATCHER || current_section == SEC_ACTIONS ||
            current_section == SEC_EVENTS  || current_section == SEC_PARSER) {
            if (!parse_property_line(line_start, NULL)) goto error;
        }
        else if (current_section == SEC_RULES) {
            if (line_start[0] == '>') {
                Rules *last_rule = current_rule - 1;

                if (!last_rule->local.is_local) {
                    last_rule->local = config.global;
                    last_rule->local.is_local = true;
                }
                if (!parse_property_line(line_start + 1, last_rule)) goto error;
            }
            else {
                if (!parse_rule_line(line_start, current_rule)) goto error;
                current_rule->local = config.global;
                current_rule++;
            }
        }

        else if (current_section == SEC_MACROS) {
            if (!parse_macro_line(line_start, current_macro, current_line)) goto error;

            current_tool->num_macros_local++;

            if      (!current_macro->has_var)           current_tool->num_pure++;
            else if (current_macro->key_1st[0] != '\0') current_tool->num_prefix++;
            else if (current_macro->key_2nd[0] == '\0') current_tool->has_wildcard = true;

            if (current_macro->value[0] == '@') current_tool->has_override = true;

            current_macro++;
        }
    }

    return true;

error:
    config.count.line_error = current_line;
    return false;
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

int set_macro_type(const Macros *macro)
{
    if (!macro->has_var)          return 0; // Pure: xy

    bool has_prefix = macro->key_1st[0] != '\0';
    bool has_suffix = macro->key_2nd[0] != '\0';

    if (has_prefix && has_suffix) return 1; // Mixed: x{VAR}y
    if (has_prefix)               return 2; // Prefix: x{VAR}
    if (has_suffix)               return 3; // Suffix: {VAR}y

    return 4; // Only variable: {VAR}
}
int comp_half(const char *half_a, const char *half_b)
{
    size_t len_a = strlen(half_a);
    size_t len_b = strlen(half_b);

    // Sort by length
    if (len_a > len_b) return -1;
    if (len_a < len_b) return 1;

    // Sort alphabetically
    return strcmp(half_a, half_b);
}
int comp_macros(const void *a, const void *b)
{
    const Macros *macro_a = (const Macros*) a;
    const Macros *macro_b = (const Macros*) b;

    int group_a = set_macro_type(macro_a);
    int group_b = set_macro_type(macro_b);

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
    if (config.count.num_rules > 1) {
        qsort(config.rules, config.count.num_rules, sizeof(Rules), comp_rules);
    }

    // Sort every tool
    if (config.count.num_tools > 1) {
        qsort(config.tools, config.count.num_tools, sizeof(Tools), comp_tools);
    }

    for (int i = 0; i < config.count.num_tools; i++) {
        if (config.tools[i].num_macros_local < 2) continue;

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

bool are_macros_equal(const Macros *macro_a, const Macros *macro_b)
{
    if (macro_a->has_var != macro_b->has_var) return false;
    if (!macro_a->has_var) return strcmp(macro_a->key, macro_b->key) == 0;

    return strcmp(macro_a->key_1st, macro_b->key_1st) == 0 &&
           strcmp(macro_a->key_2nd, macro_b->key_2nd) == 0;
}

bool are_macros_ambiguous(const Macros *macro_a, const Macros *macro_b)
{
    if (!macro_a->has_var || !macro_b->has_var) return false;

    size_t prefix_len_a   = strlen(macro_a->key_1st);
    size_t prefix_len_b   = strlen(macro_b->key_1st);

    size_t smaller_prefix = prefix_len_a;
    if (prefix_len_a > prefix_len_b) smaller_prefix = prefix_len_b;

    // compare from the start
    if (strncmp(macro_a->key_1st, macro_b->key_1st, smaller_prefix) != 0) return false;

    size_t suffix_len_a   = strlen(macro_a->key_2nd);
    size_t suffix_len_b   = strlen(macro_b->key_2nd);

    size_t smaller_suffix = suffix_len_a;
    if (suffix_len_a > suffix_len_b) smaller_suffix = suffix_len_b;

    // align both by the end
    const char *suffix_end_a = macro_a->key_2nd + (suffix_len_a - smaller_suffix);
    const char *suffix_end_b = macro_b->key_2nd + (suffix_len_b - smaller_suffix);

    // compare from the end
    if (strcmp(suffix_end_a, suffix_end_b) != 0) return false;

    int diff_prefix = (int)prefix_len_a - (int)prefix_len_b;
    int diff_suffix = (int)suffix_len_a - (int)suffix_len_b;

    return (diff_prefix * diff_suffix) < 0;
}

int line_ahead(int a, int b)
{
    if (a > b) return a;
    else       return b;
}

bool validate_macros()
{
    for (int t = 0; t < config.count.num_tools; t++) {
        const Tools *tool = &config.tools[t];

        // Do not allow two sections with the same name
        if (t > 0 && strcmp(tool->name, config.tools[t - 1].name) == 0) {
            config.count.line_error = line_ahead(tool->ini_line, config.tools[t - 1].ini_line);
            return false;
        }

        // Must have at least 2 macros to check
        if (tool->num_macros_local < 2) continue;

        // Compare already sorted pure macros
        for (int i = 0; i < tool->num_macros_local - 1; i++) {
            if (are_macros_equal(&tool->macros[i], &tool->macros[i + 1])) {
                config.count.line_error = line_ahead(tool->macros[i].ini_line, tool->macros[i + 1].ini_line);
                return false;
            }
        }

        if (config.macro_match != MAC_STRICT) continue;

        int start = tool->num_pure;
        int end   = tool->num_macros_local - tool->has_wildcard;

        int suffix_start = start + tool->num_prefix;
        int num_suffix   = end - suffix_start;

        if (start >= end) continue;
        bool has_mixed = (set_macro_type(&tool->macros[start]) == 1);
        if (!has_mixed && (tool->num_prefix == 0 || num_suffix == 0)) continue;

        for (int i = start; i < suffix_start; i++) {
            for (int j = i + 1; j < end; j++) {
                if (are_macros_ambiguous(&tool->macros[i], &tool->macros[j])) {
                    config.count.line_error = line_ahead(tool->macros[i].ini_line, tool->macros[j].ini_line);
                    return false;
                }
            }
        }
    }

    return true;
}

bool load_config(const char *filepath)
{
    set_config_default();

    FILE *ini = open_file(filepath, "r");
    if (ini == NULL) {
        config.count.line_error = -1;
        return false;
    }

    config.count = count_and_validate(ini);
    if (config.count.line_error > 0) goto error;

    // All counted, time to allocate
    if (config.count.num_rules > 0) {
        config.rules = calloc(config.count.num_rules, sizeof(Rules));
        if (config.rules == NULL) {config.count.line_error = -1; goto error;}
    }

    if (config.count.num_tools > 0) {
        config.tools = calloc(config.count.num_tools, sizeof(Tools));
        if (config.tools == NULL) {config.count.line_error = -1; goto error;}
    }

    if (config.count.num_macros > 0) {
        config.macros = calloc(config.count.num_macros, sizeof(Macros));
        if (config.macros == NULL) {config.count.line_error = -1; goto error;}
    }

    rewind(ini);
    if (!fill_config_data(ini)) goto error;

    sort_config_file();
    if (!validate_macros()) goto error;

    link_pointers();
    fclose(ini);
    return true;

error:
    free_config_struct();
    fclose(ini);
    return false;
}