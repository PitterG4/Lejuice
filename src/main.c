#include "types.h"
#include "config.h"
#include "watcher.h"

// CLI logic -- reaally wip
int main(int argc, char **argv) {
    // Case 1: only 'lejuice'
    if (argc < 2) {
        // TODO: Shop help
        return SUCCESS;
    }

    // Case 2: 'lejuice run'
    if (strcmp(argv[1], "run") == 0) {
        Config config = {0};

        // TODO: Get default OS path to config.ini
        const char *config_path = "config.ini";
        ReturnCode status = load_config("config.ini", &config);

        if (status != SUCCESS) {
            if (status == ERR_FILE_NOT_FOUND) {
                fprintf(stderr, "Error: Config file not found at %s\n", config_path);
                // TODO: It can request a default template of config file here
            } else if (status == ERR_NO_MEMORY) {
                fprintf(stderr, "Error: Memory allocation failed during setup.\n");
                // TODO: Something
            }

            return status;
        }

        printf("Starting in the background...\n");

        start_linux_watcher(config.watch_dir, 0, &config);
        free_config_struct(config);
    }

    // Case 3: 'lejuice watch <dir>' -- sets target directory
    else if (strcmp(argv[1], "watch") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: Missing directory path.\n");
            printf("Usage: lejuice watch [dir]\n");
            return ERR_INVALID_PARAM;
        }

        char *target_dir = argv[2];
        // TODO: Find global section and update watch_dir
        printf("Set to watch: %s\n", target_dir);
    }

    // Default: Unknown command
    else {
        fprintf(stderr, "Error: Unknown command line parameter.\n");
        // TODO: show help
    }
}