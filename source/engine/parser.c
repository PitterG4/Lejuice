#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "config.h"
#include "queue.h"
#include "system.h"

#define STARTS_WITH(text, prefix) (strncmp(text, prefix, sizeof(prefix) - 1) == 0)
#define REPLACE_TAG(tag, replacement) {                                                 \
    copy += snprintf(copy, MAX_CMD_LEN - (copy - final_command), "%s", (replacement));  \
    original += sizeof(tag) - 1;                                                        \
}
#define REPLACE_TAG_LEN(tag, str_len, replacement) {                                                \
    copy += snprintf(copy, MAX_CMD_LEN - (copy - final_command), "%.*s", (str_len), (replacement)); \
    original += sizeof(tag) - 1;                                                                    \
}

// Main parser

bool is_valid_rename(const FileEvent *file_event, CooldownQueue *cooldown_queue)
{
    // TODO: add prefix and sufix blacklist
    if (file_event->new_filename[0] == '.' || file_event->new_filename[0] == '~' ||
        file_event->old_filename[0] == '.' || file_event->old_filename[0] == '~') {
        return false;
    }

    // the full name changed?
    if (strcmp(file_event->old_filename, file_event->new_filename) == 0) return false;

    // same directory?
    if (file_event->old_dir_len != file_event->new_dir_len) return false;
    if (strncmp(file_event->old_filepath, file_event->new_filepath, file_event->old_dir_len) != 0) return false;

    // safety measure to avoid infinite loop
    if (!search_queue_cooldown(cooldown_queue, file_event->new_filename)) return false;

    // there is the escape prefix?
    size_t escape_len = strlen(config.escape_prefix);
    if (escape_len > 0 && strncmp(file_event->new_filename, config.escape_prefix, escape_len) == 0) {
        const char *escless_name = file_event->new_filename + escape_len;

        char escless_path[MAX_PATH_LEN];
        snprintf(escless_path, MAX_PATH_LEN, "%.*s%s", file_event->new_dir_len, file_event->new_filepath, escless_name);

        enqueue_cooldown(cooldown_queue, escless_name);
        rename_file(file_event->new_filepath, escless_path);
        return false;
    }

    return true;
}
bool is_valid_extension(const char *dot_old, const char *dot_new)
{
    if (dot_old == NULL && dot_new == NULL) return false;

    if (dot_new != NULL) {
        if (strcmp(dot_new, ".tmp") == 0 || strcmp(dot_new, ".temp") == 0 ||
            strcmp(dot_new, ".crdownload") == 0 || strcmp(dot_new, ".part") == 0) return false;
    }

    if (dot_old != NULL) {
        if (strcmp(dot_old, ".tmp") == 0 || strcmp(dot_old, ".temp") == 0 ||
            strcmp(dot_old, ".crdownload") == 0 || strcmp(dot_old, ".part") == 0) return false;
    }

    return true;
}
bool are_parameters_valid(const char *dot_old, const char *dot_new, const char *brace_start, const char *brace_end)
{
    bool has_braces = (brace_start != NULL || brace_end != NULL);

    if (has_braces) {
        // both braces exists in the right order and are not empty?
        if (brace_start == NULL || brace_end == NULL || brace_start + 1 >= brace_end) return false;

        // there are duplicated braces?
        if (strchr(brace_start + 1, '{') != NULL || strchr(brace_end + 1, '}') != NULL) return false;

        // are the braces correctly positioned?
        if (dot_new != NULL && brace_end != (dot_new - 1)) return false;
        if (dot_new == NULL && *(brace_end + 1) != '\0') return false;
    }

    else {
        // if there are no braces, ext must be different
        if (dot_new == NULL) return false;
        if (dot_old != NULL && strcmp(dot_old, dot_new) == 0) return false;
    }

    return true;
}
bool parse_file_event(FileEvent *file_event, CooldownQueue *cooldown_queue)
{
    if (!is_valid_rename(file_event, cooldown_queue)) return false;

    char *dot_old = strrchr(file_event->old_filename, '.');
    char *dot_new = strrchr(file_event->new_filename, '.');
    char *brace_start   = strchr(file_event->new_filename, '{');
    char *brace_end     = strchr(file_event->new_filename, '}');
    // TODO: Make params delimiter customizable?

    if (!is_valid_extension(dot_old, dot_new)) return false;
    if (!are_parameters_valid(dot_old, dot_new, brace_start, brace_end)) return false;

    int base_len = 0;
    if (brace_start != NULL) {
        // Base name ends where the params begin
        base_len        = (int)(brace_start - file_event->new_filename);
        int param_size  = (int)(brace_end - (brace_start + 1));

        if (param_size >= MAX_PARAMS_LEN) return false;
        snprintf(file_event->inline_params, MAX_PARAMS_LEN, "%.*s", param_size, brace_start + 1);
    }
    else {
        // If there are no params base name ends at the extension dot
        base_len = (int)(dot_new - file_event->new_filename);
    }

    // Extract the old base name
    if (dot_old != NULL) {
        int old_base_len = (int)(dot_old - file_event->old_filename);
        snprintf(file_event->old_base_name, MAX_FILENAME_LEN, "%.*s", old_base_len, file_event->old_filename);
    }
    else snprintf(file_event->old_base_name, MAX_FILENAME_LEN, "%s", file_event->old_filename);

    // Extract the new base name
    if (base_len > 0) snprintf(file_event->new_base_name, MAX_FILENAME_LEN, "%.*s", base_len, file_event->new_filename);
    else snprintf(file_event->new_base_name, MAX_FILENAME_LEN, "%s", file_event->old_base_name);

    // Extract the extensions
    if (dot_old != NULL) snprintf(file_event->old_ext, MAX_EXT_LEN, "%s", dot_old + 1);
    if (dot_new != NULL) snprintf(file_event->new_ext, MAX_EXT_LEN, "%s", dot_new + 1);
    else file_event->new_ext[0] = '\0';

    snprintf(file_event->out_filepath, MAX_PATH_LEN, "%.*s%s.%s",
             file_event->old_dir_len, file_event->new_filepath, // 'C:\videos\'
             file_event->new_base_name,                         // 'video'
             file_event->new_ext);                              // 'mp4'

    return true;
}

// Other parsing related functions

Tag tag_search(const char *token) {
    // Main tags
    if (STARTS_WITH(token, "${IN}"))        return TAG_IN;
    if (STARTS_WITH(token, "${OUT}"))       return TAG_OUT;
    if (STARTS_WITH(token, "${ARGS}"))      return TAG_ARGS;
    if (STARTS_WITH(token, "$\\") ||
        STARTS_WITH(token, "$/"))           return TAG_INSTALL;

    // Other optional tags
    if (STARTS_WITH(token, "${DIR}"))       return TAG_DIR;
    if (STARTS_WITH(token, "${PARAMS}"))    return TAG_PARAMS;

    if (STARTS_WITH(token, "${IN_NAME}"))   return TAG_IN_NAME;
    if (STARTS_WITH(token, "${IN_BASE}"))   return TAG_IN_BASE;
    if (STARTS_WITH(token, "${IN_EXT}"))    return TAG_IN_EXT;

    if (STARTS_WITH(token, "${OUT_NAME}"))  return TAG_OUT_NAME;
    if (STARTS_WITH(token, "${OUT_BASE}"))  return TAG_OUT_BASE;
    if (STARTS_WITH(token, "${OUT_EXT}"))   return TAG_OUT_EXT;
    return TAG_NONE;
}

bool extract_var_macro(const char *token, const Macros *macro, char *translated_token)
{
    size_t len_token = strlen(token);
    size_t len1st = strlen(macro->key_1st);
    size_t len2nd = strlen(macro->key_2nd);

    if (len_token <= len1st + len2nd) return false;
    if (len1st > 0 && strncmp(token, macro->key_1st, len1st) != 0) return false;
    if (len2nd > 0 && strncmp(token + len_token - len2nd, macro->key_2nd, len2nd) != 0) return false;

    int var_length = (int)(len_token - len1st - len2nd);
    char var_str[MAX_PARAMS_LEN] = {0};
    snprintf(var_str, MAX_PARAMS_LEN, "%.*s", var_length, token + len1st);

    const char *original = macro->value;
    char *copy = translated_token;

    while (*original != '\0') {
        if (STARTS_WITH(original, "{VAR}")) {
            copy += snprintf(copy, MAX_CMD_LEN - (copy - translated_token), "%s", var_str);
            original += sizeof("{VAR}") - 1;
        }
        else {
            *copy = *original;
            copy++;
            original++;
        }
    }

    *copy = '\0';
    return true;
}

Macros *find_pure_macro(Tools *tool, const char *token)
{
    if (tool->num_pure <= 0) return NULL;

    Macros matched_macro = {.has_var = false};
    snprintf(matched_macro.key, MAX_PARAMS_LEN, "%s", token);

    return bsearch(&matched_macro, tool->macros, tool->num_pure, sizeof(Macros), comp_macros);
}

// To understand this keep in mind how the macro are sorted:
// [pure]->[mixed]->[prefix_only]->[suffix only]->[wildcard]
Macros *translate_macro(const char *token, Tools *tool, MacroMatch match_type, char *translated_token)
{
    if (tool == NULL) return NULL;

    Macros *pure_found = find_pure_macro(tool, token);

    // Found a pure macro
    if (pure_found != NULL) {
        snprintf(translated_token, MAX_CMD_LEN, "%s", pure_found->value);
        return pure_found;
    }

    Macros *macro_found = NULL;
    uint16_t num_macros_var = tool->num_macros_local - tool->num_pure;
    uint16_t first_suffix = tool->num_pure + tool->num_prefix;

    // Searches for macros with 'VAR'
    if (num_macros_var < 1) return NULL;

    if (match_type == MAC_SUFFIX) {
        for (int i = first_suffix; i < tool->num_macros_local - tool->has_wildcard; i++) {
            macro_found = &tool->macros[i];
            if (extract_var_macro(token, macro_found, translated_token)) return macro_found;
        }

        for (int i = tool->num_pure; i < first_suffix; i++) {
            macro_found = &tool->macros[i];
            if (extract_var_macro(token, macro_found, translated_token)) return macro_found;
        }
    }

    else if (match_type == MAC_STRICT || match_type == MAC_PREFIX) {
        for (int i = tool->num_pure; i < tool->num_macros_local - tool->has_wildcard; i++) {
            macro_found = &tool->macros[i];
            if (extract_var_macro(token, macro_found, translated_token)) return macro_found;
        }
    }

    if (tool->has_wildcard) {
        Macros *wildcard = &tool->macros[tool->num_macros_local - 1];
        if (extract_var_macro(token, wildcard, translated_token)) return wildcard;
    }

    return NULL;
}

char *get_next_token(const char *input, char *token_out)
{
    char *next = strpbrk(input, " ,\t");

    if (next != NULL) {
        snprintf(token_out, (next - input) + 1, "%s", input);
        while (*next == ' ' || *next == ',' || *next == '\t') next++;
    }
    else snprintf(token_out, MAX_PARAMS_LEN, "%s", input);

    return next;
}

// Eg: input = "mute 720p", tool = ffmpeg (struct)
// Returns in the out_macros: "-an -vf scale=-1:720"
// Returns in the out_template the whole command in case it's an override macro
bool expand_macros(const char *input, Rules *rule, char *out_macros, char *out_template)
{
    if (input == NULL || input[0] == '\0') return false;

    Tools *active_override = NULL;
    char token[MAX_PARAMS_LEN] = {0};

    while (input != NULL && input[0] != '\0') {
        input = get_next_token(input, token);

        if (token[0] == '#') {
            int flag = set_replace_tag(token + 1);
            if (flag >= 0) {rule->local.replace = flag; continue;}

            flag = set_macro_match_tag(token + 1);
            if (flag >= 0) {rule->local.fallback = flag; continue;}

            return false;
        }

        Macros *matched = NULL;
        char translated_token[MAX_CMD_LEN] = {0};

        if (active_override != NULL) matched = translate_macro(token, active_override, config.macro_match, translated_token);
        if         (matched == NULL) matched = translate_macro(token, rule->tool, config.macro_match, translated_token);
        if         (matched == NULL) return false; // TODO: DISPATCH_EVENT

        if (translated_token[0] == '@') {
            snprintf(out_template, MAX_CMD_LEN, "%s", translated_token);

            if (matched != NULL && matched->override != NULL) active_override = matched->override;
        }
        else {
            char *end = strchr(out_macros, '\0');
            snprintf(end, MAX_PARAMS_LEN - (end - out_macros), "%s ", translated_token);
        }
    }

    return true;
}

bool parse_command(Rules *rule, FileEvent *file_event, char *final_command)
{
    char final_params[MAX_PARAMS_LEN] = {0};
    char override_template[MAX_CMD_LEN] = {0};
    char *copy = final_command;

    if (rule->tool != NULL && file_event->inline_params[0] != '\0') {
        if (!expand_macros(file_event->inline_params, rule, final_params, override_template)) return false;
    }
    const char *original;
    if (override_template[0] == '@') original = override_template + 1;
    else (original = rule->command_template);

    while (*original != '\0') {
        Tag current_tag = TAG_NONE;

        if (*original == '$') {
            current_tag = tag_search(original);
        }

        switch (current_tag) {
            case TAG_IN:        REPLACE_TAG("${IN}",        file_event->old_filepath);  break;
            case TAG_OUT:       REPLACE_TAG("${OUT}",       file_event->out_filepath);  break;
            case TAG_ARGS:      REPLACE_TAG("${ARGS}",      final_params);              break;

            case TAG_INSTALL:   REPLACE_TAG("$/",           config.install_path);       break;
            case TAG_PARAMS:    REPLACE_TAG("${PARAMS}",    file_event->inline_params); break;
            case TAG_DIR:       REPLACE_TAG_LEN("${DIR}",   file_event->old_dir_len, file_event->old_filepath); break;

            case TAG_IN_NAME:   REPLACE_TAG("${IN_NAME}",   file_event->old_filename);  break;
            case TAG_IN_BASE:   REPLACE_TAG("${IN_BASE}",   file_event->old_base_name); break;
            case TAG_IN_EXT:    REPLACE_TAG("${IN_EXT}",    file_event->old_ext);       break;

            case TAG_OUT_NAME:  REPLACE_TAG("${OUT_NAME}",  file_event->new_filename);  break;
            case TAG_OUT_BASE:  REPLACE_TAG("${OUT_BASE}",  file_event->new_base_name); break;
            case TAG_OUT_EXT:   REPLACE_TAG("${OUT_EXT}",   file_event->new_ext);       break;

            default: {
                *copy = *original;
                copy++;
                original++;
                break;
            }
        }
    }

    *copy = '\0';

    return true;
}