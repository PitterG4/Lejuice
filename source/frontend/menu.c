#include <stdio.h>
#include <string.h>

#include "types.h"
#include "system.h"

bool first_run_setup(char *out_install_path)
{
    printf("Welcome to Lejuice!\n");
    printf("Where should we set up?\n\n");

    printf("Press [Enter] for the default: (Documents\\Lejuice)\n");
    printf("  [ 1 ] - Portable mode (Current folder)\n");
    printf("  [ 2 ] - Choose a custom folder\n\n> ");

    char input[MAX_PATH_LEN];
    if (fgets(input, sizeof(input), stdin) == NULL) return false;
    input[strcspn(input, "\r\n")] = 0;

    // Case 1: [Enter]
    if (strlen(input) == 0) {
        if (!get_install_path_default(out_install_path)) return false;
    }

    // Case 2: Portable
    else if (strcmp(input, "1") == 0) {
        if (!get_exe_dir(out_install_path)) return false;
        printf("\nPortable mode activated.\n");
    }

    // Case 3: Custom path
    else if (strcmp(input, "2") == 0) {
        printf("\nCustom installation:\n");
        printf("Drag and drop a folder here, or paste the path:\n> ");

        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\r\n")] = 0;

        char custom_path[MAX_PATH_LEN];

        if (input[0] == '"') {
            char *end_quotes = strrchr(input, '"');
            if (end_quotes != NULL) *end_quotes = '\0';

            size_t len = snprintf(custom_path, MAX_PATH_LEN, "%s\\Lejuice", input + 1);
            if (len < 0 || len >= sizeof(custom_path)) return false;
        }
        else {
            size_t len = (snprintf(custom_path, MAX_PATH_LEN, "%s\\Lejuice", input));
            if (len < 0 || len >= sizeof(custom_path)) return false;
        }

        printf("\nWill be installed to: %s\n", custom_path);
        printf("Press [Enter] to confirm, or any other key to edit...\n> ");

        int confirm = getchar();
        if (confirm == '\n') {
            snprintf(out_install_path, MAX_PATH_LEN, "%s", custom_path);
            printf("\nInstalled!\n");
        }
        else {
            (printf("\nSetup cancelled.\n"));
            return false;
        }
    }

    return true;
}