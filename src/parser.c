#include "types.h"
#include <string.h>

void parse(const char *filepath, FilenameInfo *info, ConversionRule *file) {
    int len_escape = (int)strlen(file->escape_seq);
    char* dot = strrchr(filepath, '.');

    char* slash = strrchr(filepath, '/');
    char* name_start = (slash != NULL) 
        ? (slash + 1) 
        : (char*)filepath;

    int name_size; // Tamanho do nome base
    if (dot != NULL && dot > name_start) {
        name_size = (int)(dot - name_start);
    } else {
        name_size = (int)strlen(name_start);
    }
    
    if (name_size >= MAX_FILENAME_LEN) {
        name_size = MAX_FILENAME_LEN - 1;
    }
    
    strncpy(info->base_name, name_start, name_size);
    info->base_name[name_size] = '\0';

    if (dot != NULL) {
        if (*(dot - 1) == '}') {
        char* brace_start = strchr(name_start, '{');
        char* brace_end = strrchr(name_start, '}');
        
            if (brace_start != NULL && 
                brace_start < brace_end) {
                int param_size = (int)(brace_end - (brace_start + 1));
                strncpy(info->inline_params, brace_start + 1, param_size);
                info->inline_params[param_size] = '\0';

                strncpy(info->base_name, name_start, (int)(name_size - param_size - 2));
                info->base_name[(int)(name_size - param_size - 2)] = '\0';
            }
        }

        // Primeimro declara a extensão antiga
        if (info->old_ext[0] == '\0') { 
            strncpy(info->old_ext, dot + 1, MAX_EXT_LEN - 1);
            info->old_ext[MAX_EXT_LEN - 1] = '\0';

        // Depois declara a extensão nova
        } else if (info->new_ext[0] == '\0' && info->old_ext[0] != '\0') { 
            strncpy(info->new_ext, dot + 1, MAX_EXT_LEN - 1);
            info->new_ext[MAX_EXT_LEN - 1] = '\0';

            if (strcmp(info->new_ext, info->old_ext) != 0)
                info->is_valid_change = true;
        }
    }

}