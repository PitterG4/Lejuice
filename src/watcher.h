#ifndef WATCHER_H
#define WATCHER_H

#include "types.h"

ReturnCode start_linux_watcher(const char *dir_path, int flags, Config *config);

#endif //WATCHER_H