#define COBJMACROS
#define IID_OUT(iid, ppv)   &(iid), (void**)&(ppv)
#define MB(x)   ((size_t) (x) << 20)

#include <stdio.h>
#include <shlobj.h>
#include <io.h>
#include <versionhelpers.h>

#include "types.h"

uint64_t current_time_ms()
{
    return GetTickCount64();
}

bool utf8_to_wide(wchar_t *wide_out, int wide_max_len, const char *utf8)
{
    if (utf8 == NULL || wide_out == NULL || wide_max_len <= 0) return false;

    int len = MultiByteToWideChar(
        CP_UTF8, 0, utf8,   // from utf8
        -1,                 // includes \0
        wide_out,           // to wide
        wide_max_len        // buffer size
    );

    return len > 0;
}

bool wide_to_utf8(char *utf8_out, int utf8_max_len, const wchar_t *wide)
{
    if (wide == NULL || utf8_out == NULL || utf8_max_len <= 0) return 0;

    int len = WideCharToMultiByte(
        CP_UTF8, 0, wide,   // from wide
        -1,                 // includes \0
        utf8_out,           // to utf8_out
        utf8_max_len,       // buffer size
        NULL, NULL
    );

    return len > 0;
}

FILE *open_file(const char *path, const char *mode)
{
    if (path == NULL || path[0] == '\0' ||
        mode == NULL || mode[0] == '\0') return NULL;

    wchar_t wide_path[MAX_PATH_LEN];
    wchar_t wide_mode[MAX_EXT_LEN];

    if (!utf8_to_wide(wide_path, MAX_PATH_LEN, path)) return NULL;
    if (!utf8_to_wide(wide_mode, MAX_EXT_LEN, mode))  return NULL;

    return _wfopen(wide_path, wide_mode);
}

bool rename_file(const char *old_filepath, const char *new_filepath)
{
    wchar_t wide_old_filepath[MAX_PATH_LEN];
    wchar_t wide_new_filepath[MAX_PATH_LEN];

    if (!utf8_to_wide(wide_old_filepath, MAX_PATH_LEN, old_filepath)) return false;
    if (!utf8_to_wide(wide_new_filepath, MAX_PATH_LEN, new_filepath)) return false;

    if (_wrename(wide_old_filepath, wide_new_filepath) != 0) return false;

    return true;
}

bool check_file(const char *filepath)
{
    wchar_t wide_filepath[MAX_PATH_LEN];
    if (!utf8_to_wide(wide_filepath, MAX_PATH_LEN, filepath)) return false;

    else return _waccess(wide_filepath, 0) == 0;
}

bool run_process(const char *command)
{
    wchar_t wide_command[MAX_CMD_LEN];
    if (!utf8_to_wide(wide_command, MAX_CMD_LEN, command)) return false;

    STARTUPINFOW si = {.cb = sizeof(si)};
    PROCESS_INFORMATION pi = {0};

    if (!CreateProcessW(NULL, wide_command, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) return false;
    DWORD wait_result = WaitForSingleObject(pi.hProcess, INFINITE);

    bool success = false;
    DWORD exit_code = 0;

    if (wait_result == WAIT_OBJECT_0) {
        if (GetExitCodeProcess(pi.hProcess, &exit_code)) success = (exit_code == 0);
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return success;
}

bool search_path_registry(char *out_path)
{
    wchar_t wide_path[MAX_PATH_LEN];
    DWORD path_size = sizeof(wide_path);

    // Read and return the path
    LONG result = RegGetValueW(
        HKEY_CURRENT_USER,
        L"Software\\Lejuice",
        L"InstallPath",
        RRF_RT_REG_SZ, NULL,    // only return strings
        wide_path,              // buffer that receive the return data
        &path_size              // max buffer size (bytes)
    );

    if (result != ERROR_SUCCESS) return false;

    return wide_to_utf8(out_path, MAX_PATH_LEN, wide_path);
}

bool set_path_registry(char *install_path)
{
    if (install_path == NULL || install_path[0] == '\0') return false;

    wchar_t wide_path[MAX_PATH_LEN];
    if (!utf8_to_wide(wide_path, MAX_PATH_LEN, install_path)) return false;

    DWORD bytes_size = (DWORD)((wcslen(wide_path) + 1) * sizeof(wchar_t));

    // Create or overwrite the key
    LONG result = RegSetKeyValueW(
        HKEY_CURRENT_USER,
        L"Software\\Lejuice",
        L"InstallPath",
        REG_SZ,             // \0 strings
        wide_path,          // new array
        bytes_size          // size of the new array
    );

    if (result != ERROR_SUCCESS) return false;

    return true;
}

bool get_exe_dir(char *out_dir)
{
    wchar_t wide_dir[MAX_PATH_LEN];
    DWORD len = GetModuleFileNameW(NULL, wide_dir, MAX_PATH_LEN);
    if (len == 0 || len >= MAX_PATH_LEN) return false;

    // Go backwards until find a backslash
    while (len > 0 && wide_dir[len - 1] != L'\\') len--;
    if (len == 0) return false;

    wide_dir[len - 1] = L'\0';
    return wide_to_utf8(out_dir, MAX_PATH_LEN, wide_dir) > 0;
}

bool has_config_file(char *dir)
{
    char config_path[MAX_PATH_LEN];
    snprintf(config_path, MAX_PATH_LEN, "%s\\config.ini", dir);

    return check_file(config_path);
}

bool get_install_path(char *out_dir)
{
    // 1st attempt: is 'config.ini' in the same folder?
    if (get_exe_dir(out_dir)
        && has_config_file(out_dir)) return true;

    // 2nd attempt: is path in the registry?
    if (search_path_registry(out_dir)
        && has_config_file(out_dir)) return true;

    // probably first use
    return false;
}

bool get_install_path_default(char *out_path)
{
    wchar_t wide_documents[MAX_PATH_LEN];
    if (SHGetFolderPathW(
            NULL, CSIDL_PERSONAL,       // points to Documents folder
            NULL, SHGFP_TYPE_CURRENT,   // folder in current use
            wide_documents) != S_OK) return false;

    if (!wide_to_utf8(out_path, MAX_PATH_LEN, wide_documents)) return false;

    size_t len = strlen(out_path);

    int written = snprintf(out_path + len, MAX_PATH_LEN - len, "\\Lejuice");
    if (written < 0 || (size_t)written >= MAX_PATH_LEN - len) return false;

    return true;
}

void clean_path_quotes(char *path)
{
    if (path == NULL || path[0] != '"') return;

    char *end = strrchr(path, '"');
    if (end != NULL) *end = '\0';

    memmove(path, path + 1, strlen(path) + 1); // shift left
}

bool check_tool(char *tool_name)
{
    wchar_t wide_tool[MAX_FILENAME_LEN];
    if (!utf8_to_wide(wide_tool, MAX_FILENAME_LEN, tool_name)) return false;

    return SearchPathW(NULL, wide_tool, L".exe", 0, NULL, NULL) > 0;
}

uint64_t get_file_size(char *path)
{
    wchar_t wide_path[MAX_PATH_LEN];
    if (!utf8_to_wide(wide_path, MAX_PATH_LEN, path)) return 0;

    WIN32_FILE_ATTRIBUTE_DATA file_info;
    if (!GetFileAttributesExW(wide_path, GetFileExInfoStandard, &file_info)) return 0;

    // return is in bytes
    return ((uint64_t)file_info.nFileSizeHigh << 32) | (uint64_t)file_info.nFileSizeLow;
}

bool has_enough_space(char *watch_dir, char *filepath)
{
    wchar_t wide_dir[MAX_PATH_LEN];
    if (!utf8_to_wide(wide_dir, MAX_PATH_LEN, watch_dir)) return false;

    ULARGE_INTEGER bytes_available;
    if (!GetDiskFreeSpaceExW(wide_dir, &bytes_available, NULL, NULL)) return false;

    if (bytes_available.QuadPart < MB(MIN_FREE_SIZE_MB)) return false;
    else {
        uint64_t file_size = get_file_size(filepath);
        if (bytes_available.QuadPart < (file_size + MB(50))) return false;
    }

    return true;
}

bool move_to_trash(char *filepath, uint32_t replace)
{
    if (filepath == NULL || filepath[0] == '\0') return false;

    wchar_t wide_filepath[MAX_PATH_LEN + 1];
    if (!utf8_to_wide(wide_filepath, MAX_PATH_LEN, filepath)) return false;

    // Initialize COM as Single Thread
    HRESULT com_init = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    IFileOperation *file_op = NULL;
    IShellItem *target = NULL;

    HRESULT status;
    BOOL aborted = TRUE;

    // Instantiate the FileOperation and attribute it to file_op
    status = CoCreateInstance(&CLSID_FileOperation, NULL, CLSCTX_ALL, IID_OUT(IID_IFileOperation, file_op));
    if (FAILED(status)) goto cleanup;

    DWORD flags = FOF_ALLOWUNDO | FOF_WANTNUKEWARNING;
    if (IsWindows8OrGreater())  flags |= FOFX_RECYCLEONDELETE;
    if (replace == REP_ASK)     flags |= FOFX_SHOWELEVATIONPROMPT;
    else if (replace == REP_TRASH) {
        if (IsWindows8OrGreater()) flags |= FOF_NO_UI | FOFX_RECYCLEONDELETE;
        else flags |= FOF_SILENT | FOF_NOERRORUI | FOF_NOCONFIRMMKDIR;
    }

    // Set the flags
    status = IFileOperation_SetOperationFlags(file_op, flags);
    if (FAILED(status)) goto cleanup;

    // Create a target in the shell with the path
    status = SHCreateItemFromParsingName(wide_filepath, NULL, IID_OUT(IID_IShellItem, target));
    if (FAILED(status)) goto cleanup;

    // Insert into the operation queue
    status = IFileOperation_DeleteItem(file_op, target, NULL);
    if (FAILED(status)) goto cleanup;

    // Execute the operation (flags included)
    status = IFileOperation_PerformOperations(file_op);
    if (FAILED(status)) goto cleanup;

    // return true if the operation was aborted
    IFileOperation_GetAnyOperationsAborted(file_op, &aborted);

cleanup:
    if      (target != NULL) IShellItem_Release(target);
    if     (file_op != NULL) IFileOperation_Release(file_op);
    if (SUCCEEDED(com_init)) CoUninitialize();

    if (aborted) return false;
    if (check_file(filepath)) return false;

    return true;
}