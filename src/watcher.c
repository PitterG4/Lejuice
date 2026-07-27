#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <inotifytools/inotifytools.h>
#include <inotifytools/inotify.h>
#include "types.h"

extern void parse_file_event(FileEvent *event);

bool is_valid_event(struct inotify_event* event) {
    if (event->len == 0 || event->name[0] == '\0') return false;
    if (event->name[0] == '.') return false;  // hidden file verification
    
    return true;
}

void run_inotify_watcher(const char* user_dir) {
    if (!inotifytools_init(0, 0, 0)) {
        fprintf(stderr, "Error: Failed to initialize inotifytools.\n %s\n", strerror(inotifytools_error()));
        return;
    }

    printf("Starting to watch \"%s\" for renames...\n", user_dir);
    if (!inotifytools_watch_recursively(user_dir, IN_MOVE | IN_CREATE)) {
        fprintf(stderr, "Error: Could not watch directory \"%s\".\n%s\n", user_dir, strerror(inotifytools_error()));
        return;
    }

    struct inotify_event* event;
    char pending_old_filepath[MAX_PATH_LEN] = {0};
    char pending_old_filename[MAX_FILENAME_LEN] = {0};
    uint32_t pending_cookie = 0; 

    while (true) {
        event = inotifytools_next_event(-1);
        if (!event) continue;

        const char *dir_path = inotifytools_filename_from_wd(event->wd);
        if (!dir_path) continue;

        if (!is_valid_event(event)) continue;
        
        if (event->mask & IN_ISDIR) {
            if (event->mask & IN_CREATE) {
                char new_dir[MAX_PATH_LEN];

                snprintf(new_dir, MAX_PATH_LEN, "%s%s/", dir_path, event->name);
                inotifytools_watch_recursively(new_dir, IN_MOVE | IN_CREATE);
            }
            continue; 
        }

        if (event->mask & IN_MOVED_FROM) {
            snprintf(pending_old_filepath, MAX_PATH_LEN, "%s%s", dir_path, event->name);
            snprintf(pending_old_filename, MAX_FILENAME_LEN, "%s", event->name);
            pending_cookie = event->cookie;
        }
        else if (event->mask & IN_MOVED_TO) {
            if (pending_cookie != 0 && event->cookie == pending_cookie) {
                FileEvent file_event;
                
                snprintf(file_event.old_filepath, MAX_PATH_LEN, "%s", pending_old_filepath);
                snprintf(file_event.old_filename, MAX_FILENAME_LEN, "%s", pending_old_filename);
                snprintf(file_event.new_filepath, MAX_PATH_LEN, "%s%s", dir_path, event->name);
                snprintf(file_event.new_filename, MAX_FILENAME_LEN, "%s", event->name);

                parse_file_event(&file_event);
            }
            pending_old_filepath[0] = '\0';
            pending_old_filename[0] = '\0';
            pending_cookie = 0;
        }
    }
}

void run_fanotify_watcher(const char* user_dir) {
    printf("WIP.");
}

// 0 -> Inotify | 1 -> Fanotify
void start_linux_watcher(const char* user_dir, int use_fanotify) {
    if (use_fanotify == 1) {
        run_fanotify_watcher(user_dir);
    } else {
        run_inotify_watcher(user_dir);
    }
}