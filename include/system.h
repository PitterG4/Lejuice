#pragma once

#include <stdbool.h>
#include <stdint.h>

uint64_t current_time_ms();
bool utf8_to_wide(wchar_t *wide_out, int wide_max_len, const char *utf8);
bool wide_to_utf8(char *utf8_out, int utf8_max_len, const wchar_t *wide);
FILE *open_file(const char *path, const char *mode);
bool rename_file(const char *old_filepath, const char *new_filepath);
bool check_file(const char *filepath);
bool run_process(const char *command);
bool search_path_registry(char *out_path);
bool set_path_registry(char *install_path);
bool get_exe_dir(char *out_dir);
bool has_config_file(char *dir);
bool get_install_path(char *out_dir);
bool get_install_path_default(char *out_path);
void clean_path_quotes(char *path);
bool check_tool(char *tool_name);
bool has_enough_space(char *watch_dir, char *filepath);
bool move_to_trash(char *filepath, uint32_t replace);