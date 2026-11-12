#include <stdio.h>
#include <time.h>

#include "system.h"
#include "types.h"
#include "tray.h"

char* flag_to_section(EventFlag flag)
{
    if (flag & FLAG_SYS_INFO)       return "[SYSTEM_INFO]";
    if (flag & FLAG_SYS_ERROR)      return "[SYSTEM_ERROR]";

    if (flag & FLAG_TASK_START)     return "[TASK_START]";
    if (flag & FLAG_TASK_DONE)      return "[TASK_DONE]";
    if (flag & FLAG_TASK_ERROR)     return "[TASK_ERROR]";

    if (flag & FLAG_FILTER_RULE)    return "[FILTER_RULE]";
    if (flag & FLAG_FILTER_NOISE)   return "[FILTER_NOISE]";

    if (flag & FLAG_DEBUG)          return "[DEBUG]";

    return "[MISC]";
}

void write_to_log(EventFlag flag, const char *title, const char *message)
{
    char log_install[MAX_PATH_LEN];
    snprintf(log_install, MAX_PATH_LEN  - 16, "%s\\lejuice.txt", config.install_path);

    FILE *log_file = open_file(log_install, "a");
    if (log_file == NULL) return;

    const char *section = flag_to_section(flag);

    char time_str[20];
    time_t now = time(NULL);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));

    fprintf(log_file, "%s %s %s: %s\n", time_str, section, title, message);
    fclose(log_file);
}

void dispatch_event(EventFlag flag, Profile local, const char *title, const char *message)
{
    if (local.notify & flag) show_notification(flag, title, message);
    if (local.log & flag) write_to_log(flag, title, message);
}