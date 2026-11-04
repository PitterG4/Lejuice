#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "menu.h"
#include "types.h"
#include "system.h"
#include "config.h"
#include "watcher.h"

bool verify_syntax(char *config_path)
{
    if (load_config(config_path)) return true;

    if (config.count.line_error > 0) printf("[ERROR] Incorrect syntax in config.ini. Line: %d.\n", config.count.line_error);
    else printf("[ERROR] Could not open '%s'.\n", config_path);

    free_config_struct();
    return false;
}

void are_tools_valid()
{
    for (int i = 0; i < config.count.num_rules; i++) {
        char *tool = config.rules[i].tool_name_temp;
        if (!check_tool(tool)) {
            printf("[WARNING] %s -> %s : '%s' not found\n",
                    config.rules[i].from_ext, config.rules[i].to_ext, tool);
        }
        else printf("[OK] %s -> %s : '%s' found\n",
                    config.rules[i].from_ext, config.rules[i].to_ext, tool);
    }
}

void print_help()
{
    printf(
        "Usage:\n"
        " - lejuice run           > Start watching the defined directory\n"
        " - lejuice stop          > Stop the running watcher\n"
        " - lejuice watch [dir]   > Set the directory to be watched\n"
        " - lejuice setup         > Run the installation setup\n"
        " - lejuice status        > Show the program's current state\n"
        " - lejuice check         > Validate the config syntax and rules\n"
    );
}

Command set_command_tag(char *name)
{
    if (strcmp(name, "run") == 0)    return CMD_RUN;
    if (strcmp(name, "stop") == 0)   return CMD_STOP;

    if (strcmp(name, "watch") == 0)  return CMD_WATCH;
    if (strcmp(name, "setup") == 0)  return CMD_SETUP;

    if (strcmp(name, "status") == 0) return CMD_STATUS;
    if (strcmp(name, "check") == 0)  return CMD_CHECK;

    if (strcmp(name, "help") == 0)  return CMD_HELP;

    return CMD_UNKNOWN;
}

int main (int argc, char **argv)
{
    // now 3 is the max args number
    if (argc < 2 || argc > 4) {
        print_help();
        return EXIT_SUCCESS;
    }

    Command command = set_command_tag(argv[1]);

    if (command == CMD_UNKNOWN) {
        fprintf(stderr, "Error: Unknown command.\n");
        return EXIT_FAILURE;
    }

    if (command == CMD_HELP) {
        print_help();
        return EXIT_SUCCESS;
    }

    if (command == CMD_STATUS) {
        if (is_watcher_running()) printf("[STATUS] Running\n");
        else                      printf("[STATUS] Stopped\n");
        return EXIT_SUCCESS;
    }

    if (command == CMD_STOP) {
        if (!stop_watcher()) {
            fprintf(stderr, "Error: Watcher unavailable or stop request failed.\n");
            return EXIT_FAILURE;
        }

        printf("Stop requested.\n");
        return EXIT_SUCCESS;
    }

    if (command == CMD_SETUP) {
        config.install_path[0] = '\0';

        if (!first_run_setup(config.install_path)) return EXIT_FAILURE;
        if (!has_config_file(config.install_path)) {
            fprintf(stderr, "Error: config.ini has not been created.\n");
            return EXIT_FAILURE;
        }
        if (!set_path_registry(config.install_path)) {
            fprintf(stderr, "Error: Could not save installation path.\n");
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }

    // 'lejuice watch [dir]'
    if (command == CMD_WATCH) {
        if (argc < 3) {
            fprintf(stderr, "Error: Missing directory path.\n"
                            "Usage: lejuice watch [dir]\n");
            return EXIT_FAILURE;
        }

        char *target_dir = argv[2];
        clean_path_quotes(target_dir);

        // TODO: Find global section and update watch_dir
        fprintf(stderr, "Error: 'watch' is not implemented yet.\n");
        return EXIT_FAILURE;
    }

    if (!get_install_path(config.install_path)) {
        fprintf(stderr,"Error: Configuration not found.\n"
                       "Use 'lejuice setup' or provide config.ini.\n");
        return EXIT_FAILURE;
    }

    char config_path[MAX_PATH_LEN];
    int config_path_len = snprintf(config_path, sizeof(config_path), "%s\\config.ini", config.install_path);

    if (config_path_len >= MAX_PATH_LEN) {
        fprintf(stderr, "Error: Configuration path is too long.\n");
        return EXIT_FAILURE;
    }

    if (!verify_syntax(config_path)) return EXIT_FAILURE;

    if (command == CMD_CHECK) {
        int exit = EXIT_SUCCESS;

        printf("[OK] Syntax valid (%d rules, %d tools, %d macros)\n",
               config.count.num_rules,
               config.count.num_tools,
               config.count.num_macros
        );

        if (config.watch_dir[0] == '\0' || !check_file(config.watch_dir)) {
            fprintf(stderr, "[ERROR] Watch path missing or unavailable: %s\n", config.watch_dir);
            exit = EXIT_FAILURE;
        }
        else printf("[OK] Watch path exists: %s\n", config.watch_dir);

        printf("\nRule dependencies:\n");
        are_tools_valid();

        free_config_struct();
        return exit;
    }

    if (command == CMD_RUN) {
        if (is_watcher_running()) {
            fprintf(stderr, "Error: Lejuice is already running.\n");
            return EXIT_FAILURE;
        }

        if (config.watch_dir[0] == '\0') {
            fprintf(stderr, "Error: Watch directory not specified in config.ini.\n"
                            "Tip: you can use 'lejuice watch [dir]\n");
            free_config_struct();
            return EXIT_FAILURE;
        }

        printf("Starting in the background...\n"
               "Watching: %s\n", config.watch_dir);

        bool success = run_watcher(config.watch_dir);
        free_config_struct();

        if (!success) {
            fprintf(stderr, "Error: Watcher failed.\n");
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }

    return EXIT_FAILURE;
}