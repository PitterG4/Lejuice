#include "types.h"
#include "config.c"

static void parse_file_event(const FileEvent *file_event) { // WIP

    // 1st layer: the name/path changed?
    if (strcmp(file_event->old_filename, file_event->new_filename) == 0) return;

    char *old_dot = strrchr(file_event->old_filename, '.');
    char *new_dot = strrchr(file_event->new_filename, '.');

    // 2nd layer: are both files extension-less?
    if (old_dot == NULL && new_dot == NULL) return;

    char *brace_start = strchr(file_event->new_filename, '{');
    char *brace_end = strchr(file_event->new_filename, '}');

    // 3rd layer: are both extension equals?
    if (old_dot != NULL && new_dot != NULL) {
        if (strcmp(old_dot + 1, new_dot + 1) == 0) {
            if (brace_start == NULL || brace_end == NULL) return;
        }
    }

    FileInfo info = {0};

    if (new_dot != NULL && new_dot > file_event->new_filename && *(new_dot - 1) == '}') {
        if (brace_start != NULL && brace_start < brace_end) {
            int param_size = brace_end - (brace_start + 1);
            snprintf(info.inline_params, MAX_PARAMS_LEN, "%.*s", param_size, brace_start + 1);
        }
    }

    if (old_dot != NULL) snprintf(info.old_ext, MAX_EXT_LEN, "%s", old_dot + 1);
    if (new_dot != NULL) snprintf(info.new_ext, MAX_EXT_LEN, "%s", new_dot + 1);
}