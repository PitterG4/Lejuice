#include <stdio.h>
#include <windows.h>

#include "types.h"
#include "engine.h"
#include "parser.h"
#include "queue.h"
#include "system.h"

// is named 'open_directory' solely for the program purpose
HANDLE open_directory(const char *path)
{
    wchar_t wide_path[MAX_PATH_LEN];
    if (!utf8_to_wide(wide_path, MAX_PATH_LEN, path)) return INVALID_HANDLE_VALUE;

    return CreateFileW(
        wide_path,                      // pointer to the dir/file name
        FILE_LIST_DIRECTORY,            // access mode (read/write)
        FILE_SHARE_READ                 // share mode
            | FILE_SHARE_WRITE
            | FILE_SHARE_DELETE,
        NULL,                           // security descriptor
        OPEN_EXISTING,                  // how it should create
        FILE_FLAG_BACKUP_SEMANTICS      // flags and attributes
            | FILE_FLAG_OVERLAPPED,
        NULL                            // dir/file with attributes to copy
    );
}

BOOL read_dir_changes(HANDLE dir_handle, OVERLAPPED *overlapped, BYTE *buffer, DWORD buffer_size)
{
    return ReadDirectoryChangesW(
        dir_handle,                     // directory handle
        buffer,                         // buffer
        buffer_size,                    // length of the buffer
        TRUE,                           // watch recursively
        FILE_NOTIFY_CHANGE_FILE_NAME,   // notify filter -- TODO: '| FILE_NOTIFY_CHANGE_DIR_NAME' maybe?
        NULL,                           // number of bytes returned
        overlapped,                     // OVERLAPPEED structure
        NULL                            // IO completion routine
    );
}

bool is_watcher_running()
{
    HANDLE watcher_event = OpenEventW(EVENT_MODIFY_STATE, FALSE, L"Lejuice");
    if (watcher_event == NULL) return false;

    CloseHandle(watcher_event);
    return true;
}

bool stop_watcher()
{
    HANDLE watcher_event = OpenEventW(EVENT_MODIFY_STATE, FALSE, L"Lejuice");
    if (watcher_event == NULL) return false;

    SetEvent(watcher_event);
    CloseHandle(watcher_event);
    return true;
}

int extract_filename(char *filepath, char *out_filename)
{
    char *backslash = strrchr(filepath, '\\');
    if (backslash != NULL) {
        snprintf(out_filename, MAX_FILENAME_LEN, "%s", backslash + 1);
        return (int)((backslash + 1) - filepath);
    }

    return 0;
}

bool run_watcher(const char *path)
{
    HANDLE dir_handle = open_directory(path);
    if (dir_handle == INVALID_HANDLE_VALUE) return false;

    OVERLAPPED overlapped = {0};
    HANDLE watcher_event = CreateEventW(NULL, TRUE, FALSE, L"Lejuice");

    if (watcher_event == NULL) {
        CloseHandle(dir_handle);
        return false;
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(watcher_event);
        CloseHandle(dir_handle);
        return false;
    }

    overlapped.hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (overlapped.hEvent == NULL) {
        CloseHandle(watcher_event);
        CloseHandle(dir_handle);
        return false;
    }

    HANDLE handles_to_wait[2] = {overlapped.hEvent, watcher_event};

    _Alignas(DWORD) BYTE buffer[BUFFER_SIZE + sizeof(WCHAR)];

    //  About queue's size:
    //      event_queue     = size
    //      cooldown_queue  = size * 2
    //  PS: minimum size is defined as 8
    if (config.queue_size < 8) config.queue_size = 8;

    EventQueue event_queue = {
        .size = config.queue_size,
        .events = malloc(config.queue_size * sizeof(FileEvent)),
    };

    CooldownQueue cooldown_queue = {
        .size = config.queue_size * 2,
        .items = calloc(config.queue_size * 2, sizeof(CooldownItem)),
    };

    bool result = false;

    char root_path[MAX_PATH_LEN] = {0};
    int root_len = snprintf(root_path, MAX_PATH_LEN, "%s", path);
    if (root_len < 0 || root_len >= MAX_PATH_LEN - 1) goto cleanup;

    if (root_len > 0 && root_path[root_len - 1] != '\\') {
        root_path[root_len++] = '\\';
        root_path[root_len] = '\0';
    }

    int relative_path_len = MAX_PATH_LEN - root_len;

    FileEvent old_event = {0};
    memcpy(old_event.old_filepath, root_path, root_len);
    memcpy(old_event.new_filepath, root_path, root_len);

    while (true)
    {
        if (!read_dir_changes(dir_handle, &overlapped, buffer, BUFFER_SIZE)) goto cleanup;

        DWORD objects_received = WaitForMultipleObjects(2, handles_to_wait, FALSE, INFINITE);
        DWORD bytes_transferred = 0;

        if (objects_received == WAIT_OBJECT_0 + 1) {
            CancelIoEx(dir_handle, &overlapped);
            GetOverlappedResult(dir_handle, &overlapped, &bytes_transferred, TRUE);

            result = true;
            goto cleanup;
        }
        if (!GetOverlappedResult(dir_handle, &overlapped, &bytes_transferred, TRUE) || bytes_transferred <= 0) continue;

        FILE_NOTIFY_INFORMATION *win_event = (FILE_NOTIFY_INFORMATION*)buffer;

        while (true)
        {
            size_t name_len = win_event->FileNameLength / sizeof(WCHAR);
            WCHAR swap_char = win_event->FileName[name_len];
            win_event->FileName[name_len] = L'\0';

            if (win_event->Action == FILE_ACTION_RENAMED_OLD_NAME) {
                wide_to_utf8(old_event.old_filepath + root_len, relative_path_len, win_event->FileName);
                old_event.old_dir_len = extract_filename(old_event.old_filepath, old_event.old_filename);
            }

            else if (win_event->Action == FILE_ACTION_RENAMED_NEW_NAME && old_event.old_filepath[root_len] != '\0') {
                wide_to_utf8(old_event.new_filepath + root_len, relative_path_len, win_event->FileName);
                old_event.new_dir_len = extract_filename(old_event.new_filepath, old_event.new_filename);

                // Add event to queue
                enqueue_event(&event_queue, &old_event);
                old_event.old_filepath[root_len] = '\0';
            }
            else old_event.old_filepath[root_len] = '\0';

            win_event->FileName[name_len] = swap_char;

            // Go to the next action in buffer
            if (win_event->NextEntryOffset == 0) break;
            win_event = (FILE_NOTIFY_INFORMATION*)((uint8_t*)win_event + win_event->NextEntryOffset);
        }

        FileEvent current_event;
        while (!queue_empty(&event_queue)) {
            current_event = dequeue_event(&event_queue);

            // Send file to parser.c
            if (parse_file_event(&current_event, &cooldown_queue)) {
                if (!has_enough_space(root_path, current_event.new_filepath)) {
                    // TODO: dispatch_event
                    continue;
                }

                // if run_engine == false -> tray
                run_engine(&current_event, &cooldown_queue);
            }
        }
    }

cleanup:
    free(event_queue.events);
    free(cooldown_queue.items);

    CloseHandle(overlapped.hEvent);
    CloseHandle(watcher_event);
    CloseHandle(dir_handle);

    return result;
}