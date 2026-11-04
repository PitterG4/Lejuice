#include <windows.h>
#include <stdio.h>

#include "types.h"
#include "config.h"
#include "queue.h"
#include "parser.h"
#include "system.h"

void resolve_collision(char *filepath, char *ext)
{
    if (!check_file(filepath)) return;

    char *target = NULL;
    bool has_ext = (ext != NULL && ext[0] != '\0');

    if (has_ext) target = strrchr(filepath, '.');
    else target = filepath + strlen(filepath);

    int counter = 0;
    int path_len = (int)(target - filepath);

    while (check_file(filepath)) {
        counter++;
        if (has_ext)    snprintf(target, MAX_PATH_LEN - path_len, " (%d).%s", counter, ext);
        else            snprintf(target, MAX_PATH_LEN - path_len, " (%d)", counter);
    }
}

bool run_engine(FileEvent *file_event, CooldownQueue *cooldown_queue)
{
    Rules rule_found = {0};
    snprintf(rule_found.from_ext, MAX_EXT_LEN, "%s", file_event->old_ext);
    snprintf(rule_found.to_ext, MAX_EXT_LEN, "%s", file_event->new_ext);

    Rules *matched_rule = bsearch(&rule_found, config.rules, config.count.num_rules, sizeof(Rules), comp_rules);
    if (matched_rule == NULL) return false;

    enqueue_cooldown(cooldown_queue, file_event->old_filename);
    if (!rename_file(file_event->new_filepath, file_event->old_filepath)) return false;

    bool restore_name = (strcmp(file_event->old_filepath, file_event->out_filepath) == 0);
    resolve_collision(file_event->out_filepath, file_event->new_ext);

    char command[MAX_CMD_LEN];
    Rules final_rule = *matched_rule;

    if (!parse_command(&final_rule, file_event, command)) return false;
    if (!run_process(command)) return false;

    if (final_rule.local.replace == REP_KEEP) return true;
    if (!move_to_trash(file_event->old_filepath, final_rule.local.replace)) return true;

    if (restore_name) {
        enqueue_cooldown(cooldown_queue, file_event->old_filename);
        rename_file(file_event->out_filepath, file_event->old_filepath);
    }

    return true;
}