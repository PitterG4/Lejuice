#pragma once

#include <comcat.h>

#include "types.h"

bool start_tray(HWND hWnd);
void show_notification(EventFlag flag, const char *title, const char *message);
void tray_cleanup();