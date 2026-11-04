#include <windows.h>

#include "types.h"

NOTIFYICONDATAW nid = {0};

bool start_tray(HWND hWnd)
{
    nid.cbSize = sizeof(NOTIFYICONDATAW);   // size of the structure
    nid.hWnd = hWnd;                        // link the tray icon to the window
    nid.uID = 0;                            // unique ID for this icon

    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;

    nid.uCallbackMessage = WM_USER + 1;     // message id sent to the window
    nid.uVersion = NOTIFYICON_VERSION_4;    // set the tray version

    nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcsncpy(nid.szTip, L"Lejuice - Watching", 128);

    if (!Shell_NotifyIconW(NIM_ADD, &nid)) return false;
    return Shell_NotifyIconW(NIM_SETVERSION, &nid);
}

void show_notification(EventFlag flag, const char *title, const char *message)
{
    nid.uFlags = NIF_INFO;

    if (flag & (FLAG_SYS_ERROR | FLAG_TASK_ERROR)) {
        nid.dwInfoFlags = NIIF_ERROR;
    }
    else if (flag & FLAG_FILTER) {
        nid.dwInfoFlags = NIIF_WARNING | NIIF_NOSOUND;
    }
    else {
        nid.dwInfoFlags = NIIF_INFO | NIIF_NOSOUND | NIIF_RESPECT_QUIET_TIME;
    }

    MultiByteToWideChar(CP_UTF8, 0, title, -1, nid.szInfoTitle, 64);
    MultiByteToWideChar(CP_UTF8, 0, message, -1, nid.szInfo, 256);

    Shell_NotifyIconW(NIM_MODIFY, &nid);
}

void tray_cleanup()
{
    Shell_NotifyIconW(NIM_DELETE, &nid);
}